import * as midlevel from "../midlevel/index.js";
import { ResourceItem, Status } from "../types/index.js";
import { Cached } from "../utils/cache.js";
import { EventEmitter } from "./events.js";
import type { BaseModule } from "./base_module.js";

export type ResourceContainerContent = midlevel.ResourceContainerContent;
export type ResourceContainerStatus = midlevel.ResourceContainerStatus;

export type TransferCallback =
    (resource: ResourceItem) => Promise<void> | void;

export type Events = {
    content: (content: ResourceContainerContent) => Promise<void> | void;
    // Emitted when the container goes offline and stops monitoring
    offline: (status: Status) => Promise<void> | void;
}

const DEFAULT_CONTENT_CACHE_MS = 250;

export class ResourceContainer extends EventEmitter<Events> implements BaseModule {
    readonly type = midlevel.ModuleType.RESOURCE_CONTAINER;
    private cached_content = new Cached<ResourceContainerContent>();
    private stopped = false;
    private loop?: Promise<void>;
    private in_callback = false;
    opened_port: [port_id: number, access_key: number] | undefined = undefined;

    constructor(
        private rpc: midlevel.ResourceContainer,
        readonly name: string,
    ) {
        super();
    }

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.RESOURCE_CONTAINER)) {
            return Status.fail("expected ResourceContainer");
        }
        await this.release();
        this.rpc = rpc;
        return await this.init();
    }

    down_level(): midlevel.ResourceContainer {
        return this.rpc;
    }

    async init(): Promise<Status> {
        this.stopped = false;
        this.loop ??= this.monitor_loop();
        return Status.ok();
    }

    async get_content(
        cache_expiring_ms: number = DEFAULT_CONTENT_CACHE_MS,
    ): Promise<[Status, ResourceContainerContent | undefined]> {
        const cached = this.cached_content.get(cache_expiring_ms);
        if (cached) {
            return [Status.ok(), cached];
        }
        const [status, content] = await this.rpc.get_content();
        if (!status.is_ok() || !content) {
            return [status, undefined];
        }
        this.cached_content.set(content);
        return [Status.ok(), content];
    }

    free_volume(): number | undefined {
        const content = this.cached_content.get(Infinity);
        if (!content) {
            return undefined;
        }
        return content.volume - content.used;
    }

    async open_port(
        access_key: number,
    ): Promise<[Status, number | undefined]> {
        const [status, port_id] = await this.rpc.open_port(access_key);
        if (status.is_ok() && port_id !== undefined) {
            this.opened_port = [port_id, access_key];
        }
        return [status, port_id];
    }

    async close_port(): Promise<Status> {
        const status = await this.rpc.close_port();
        if (status.is_ok()) {
            this.opened_port = undefined;
        }
        return status;
    }

    async transfer(
        port_id: number,
        access_key: number,
        resource: ResourceItem,
        progress_cb?: TransferCallback,
    ): Promise<Status> {
        const status = await this.rpc.transfer(
            port_id,
            access_key,
            resource,
            async (item) => {
                this.cached_content.reset();
                if (progress_cb) {
                    await progress_cb(item);
                }
            },
        );
        this.cached_content.reset();
        return status;
    }

    async release(): Promise<Status> {
        this.stopped = true;
        if (this.loop && !this.in_callback) {
            await this.loop;
            this.loop = undefined;
        }
        this.cached_content.reset();
        this.opened_port = undefined;
        return Status.ok();
    }

    private async monitor_loop(): Promise<void> {
        while (!this.stopped) {
            try {
                const status = await this.rpc.monitoring(async (content) => {
                    await this.apply_content(content);
                    return !this.stopped;
                });
                if (!status.is_ok()) {
                    await this.notify_offline(status);
                    return;
                }
            } catch (error) {
                await this.notify_offline(Status.exception(error));
                return;
            }
        }
    }

    private async notify_offline(status: Status): Promise<void> {
        this.loop = undefined;
        this.in_callback = true;
        try {
            await this.emit("offline", status);
        } finally {
            this.in_callback = false;
        }
    }

    private async apply_content(content: ResourceContainerContent): Promise<void> {
        const previous = this.cached_content.get(Infinity);
        this.cached_content.set(content);
        if (previous && previous.timestamp === content.timestamp) {
            return;
        }
        this.in_callback = true;
        try {
            await this.emit("content", content);
        } finally {
            this.in_callback = false;
        }
    }

}
