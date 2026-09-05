import * as midlevel from "#sdk/midlevel/index.js";
import { Status } from "#sdk/types/index.js";
import { Cached } from "#sdk/utils/cache.js";
import type { BaseModule } from "./base_module.js";

export type MessangerStatus = midlevel.MessangerStatus;
export type MessangerRequest = midlevel.MessangerRequest;

export interface RequestHandler {
    handler(request: MessangerRequest): Promise<string> | string;
    offline(): Promise<void> | void;
}

const DEFAULT_SERVICES_CACHE_MS = 500;

type HostedService = {
    name: string;
    handler: RequestHandler;
    stopped: boolean;
    service: midlevel.MessangerService | undefined;
    loop: Promise<void>;
};

export class Messanger implements BaseModule {
    readonly type = midlevel.ModuleType.MESSANGER;
    private services_list = new Cached<string[]>();
    private hosted = new Map<string, HostedService>();
    private stopped = false;

    constructor(
        private rpc: midlevel.Messanger,
        readonly name: string,
    ) {}

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.MESSANGER)) {
            return Status.fail("expected Messanger");
        }
        await this.release();
        this.rpc = rpc;
        this.stopped = false;
        return Status.ok();
    }

    down_level(): midlevel.Messanger {
        return this.rpc;
    }

    async get_services_list(
        cache_expiring_ms: number = DEFAULT_SERVICES_CACHE_MS,
    ): Promise<[Status, string[] | undefined]> {
        const cached = this.services_list.get(cache_expiring_ms);
        if (cached) {
            return [Status.ok(), cached];
        }
        const [status, listed] = await this.rpc.services_list();
        if (!status.is_ok() || !listed) {
            return [status, undefined];
        }
        this.services_list.set(listed.services);
        return [Status.ok(), listed.services];
    }

    async send_request(
        service: string,
        body: string,
        timeout_ms: number = 1000,
    ): Promise<[Status, string | undefined]> {
        return this.rpc.send_request(service, body, timeout_ms);
    }

    async serve(
        service_name: string,
        handler: RequestHandler,
        force: boolean = false,
    ): Promise<Status> {
        if (this.stopped) {
            return Status.fail("messanger is released");
        }
        const existing = this.hosted.get(service_name);
        if (existing) {
            // reject even if force is true
            return Status.fail(`service ${service_name} already exists hosted locally`);
        }
        const [status, service] = await this.rpc.open_service(service_name, force);
        if (!status.is_ok() || !service) {
            return status.wrap("failed to open service");
        }
        this.remember_service(service_name);
        const hosted: HostedService = {
            name: service_name,
            handler,
            stopped: false,
            service,
            loop: Promise.resolve(),
        };
        this.hosted.set(service_name, hosted);
        hosted.loop = this.serve_loop(hosted);
        return Status.ok();
    }

    async stop_serve(service_name: string): Promise<Status> {
        const hosted = this.hosted.get(service_name);
        if (!hosted) {
            return Status.fail(`service ${service_name} is not hosted locally`);
        }
        await this.close_service(hosted);
        return Status.ok();
    }

    async release(): Promise<Status> {
        this.stopped = true;
        await this.rpc.terminate();
        const hosted = [...this.hosted.values()];
        this.hosted.clear();
        await Promise.all(hosted.map((entry) => this.close_service(entry)));
        this.services_list.reset();
        return Status.ok();
    }

    private async serve_loop(hosted: HostedService): Promise<void> {
        try {
            while (!hosted.stopped && !this.stopped) {
                if (!hosted.service) {
                    return;
                }
                const [status, request] = await hosted.service.wait_request();
                if (hosted.stopped || this.stopped) {
                    return;
                }
                if (!status.is_ok() || !request) {
                    await this.close_service(hosted);
                    return;
                }
                try {
                    const body = await hosted.handler.handler(request);
                    if (!hosted.stopped && !this.stopped && hosted.service) {
                        await hosted.service.send_response(request, body);
                    }
                } catch {
                    // Drop the request; the client will time out.
                }
            }
        } finally {
            await this.close_service(hosted);
        }
    }

    private remember_service(name: string): void {
        const services = this.services_list.get(Infinity);
        if (!services || services.includes(name)) {
            return;
        }
        this.services_list.set([...services, name]);
    }

    private forget_service(name: string): void {
        const services = this.services_list.get(Infinity);
        if (!services || !services.includes(name)) {
            return;
        }
        this.services_list.set(
            services.filter((service) => service !== name)
        );
    }

    private async close_service(hosted: HostedService): Promise<void> {
        const service = hosted.service;
        this.forget_service(hosted.name);
        hosted.stopped = true;
        hosted.service = undefined;
        if (service) {
            await service.close();
        }
        try {
            hosted.handler.offline();
        } catch {
            // Handler errors must not block shutdown.
        }
    }
}
