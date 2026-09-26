import { Status } from "@spx/sdk/types";
import { Logger } from "../log.js";
import { RetryTimeout } from "../utils/retry_timeout.js";

export type MonitoredRemote<T> = {
    monitoring(
        callback: (value: T | undefined) => Promise<boolean>,
        heartbeat_ms?: number,
    ): Promise<Status>;
    terminate(): Promise<void>;
};

export class ModuleMonitor<T> {
    private stopped = false;
    private task?: Promise<void>;

    constructor(
        private readonly remote: MonitoredRemote<T>,
        private readonly logger: Logger,
        private readonly name: string,
        private readonly apply: (value: T) => void,
    ) {}

    start(): void {
        this.task = this.run();
    }

    async stop(): Promise<void> {
        this.stopped = true;
        await this.remote.terminate();
        await this.task;
    }

    private async run(): Promise<void> {
        const retry_timeout = new RetryTimeout([500, 1000, 2000, 5000]);
        while (!this.stopped) {
            const status = await this.remote.monitoring(async (value) => {
                if (this.stopped) {
                    return false;
                }
                if (value !== undefined) {
                    this.apply(value);
                }
                return true;
            });
            if (!this.stopped && !status.is_ok()) {
                this.logger.error(
                    `Module '${this.name}' monitoring failed: ${status.what()}`,
                );
                await retry_timeout.wait_to_retry(() => this.stopped);
            } else {
                retry_timeout.reset();
            }
        }
    }
}
