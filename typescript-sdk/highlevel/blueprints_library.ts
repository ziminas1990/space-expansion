import * as midlevel from "#sdk/midlevel/index.js";
import { Blueprint, Status } from "#sdk/types/index.js";
import { Cached } from "#sdk/utils/cache.js";
import type { BaseModule } from "./base_module.js";

export type { Blueprint };

const DEFAULT_LIST_CACHE_MS = 250;

export class BlueprintsLibrary implements BaseModule {
    readonly type = midlevel.ModuleType.BLUEPRINTS_LIBRARY;
    private names = new Cached<string[]>();
    private blueprints = new Map<string, Blueprint>();

    constructor(
        private rpc: midlevel.BlueprintsLibrary,
        readonly name: string,
    ) {}

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.BLUEPRINTS_LIBRARY)) {
            return Status.fail("expected BlueprintsLibrary");
        }
        await this.release();
        this.rpc = rpc;
        return Status.ok();
    }

    down_level(): midlevel.BlueprintsLibrary {
        return this.rpc;
    }

    async get_blueprints_list(
        start_with: string = "",
        cache_expiring_ms: number = DEFAULT_LIST_CACHE_MS,
    ): Promise<[Status, string[] | undefined]> {
        let names = this.names.get(cache_expiring_ms);
        if (!names) {
            const [status, fetched] = await this.rpc.get_blueprints_list();
            if (!status.is_ok() || !fetched) {
                return [status, undefined];
            }
            this.names.set(fetched);
            names = fetched;
        }
        if (start_with) {
            return [Status.ok(), names.filter((name) => name.startsWith(start_with))];
        }
        return [Status.ok(), names];
    }

    async get_blueprint(
        name: string,
        reset_cached: boolean = false,
    ): Promise<[Status, Blueprint | undefined]> {
        if (reset_cached) {
            this.blueprints.delete(name);
        } else {
            const cached = this.blueprints.get(name);
            if (cached) {
                return [Status.ok(), cached];
            }
        }
        const [status, blueprint] = await this.rpc.get_blueprint(name);
        if (!status.is_ok() || !blueprint) {
            return [status, undefined];
        }
        this.blueprints.set(name, blueprint);
        return [Status.ok(), blueprint];
    }

    async release(): Promise<Status> {
        await this.rpc.terminate();
        this.names.reset();
        this.blueprints.clear();
        return Status.ok();
    }
}
