import { Status } from "#sdk/types/status.js";

export class EventEmitter<
    Events extends { [K in keyof Events]: (...args: any[]) => Promise<void> | void }
> {
    private listeners: { [K in keyof Events]?: Events[K][] } = {};

    on<K extends keyof Events>(event: K, listener: Events[K]): Status {
        const handlers = this.listeners[event];
        if (handlers) {
            handlers.push(listener);
        } else {
            this.listeners[event] = [listener];
        }
        return Status.ok();
    }

    off<K extends keyof Events>(event: K, listener: Events[K]): Status {
        const handlers = this.listeners[event];
        if (!handlers) {
            return Status.ok();
        }
        this.listeners[event] = handlers.filter((item) => item !== listener) as Events[K][];
        return Status.ok();
    }

    protected async emit<K extends keyof Events>(
        event: K,
        ...params: Parameters<Events[K]>
    ): Promise<void>
    {
        const handlers = this.listeners[event];
        if (!handlers) {
            return;
        }
        for (const listener of handlers) {
            await (listener as (...args: Parameters<Events[K]>) => Promise<void> | void)(
                ...params);
        }
    }
}
