import * as midlevel from "@spx/sdk/midlevel";
import { Status } from "@spx/sdk/types";
import { local_now_us } from "@spx/sdk/utils";
import { IWorld } from "./interfaces.js";
import { Logger } from "../log.js";
import { RetryTimeout } from "../utils/retry_timeout.js";

const MONITOR_INTERVAL_MS = 40;
const MONITOR_RETRY_MS = [500, 1000, 2000, 5000];

export class SystemClock {

    private stopped: boolean = false;
    private stop_task: Promise<Status> | undefined;
    private monitoring_task?: Promise<void>;

    constructor(
        private readonly remote: midlevel.SystemClock,
        private readonly world: IWorld,
        private readonly logger: Logger,
        private readonly name: string,
    ) {}

    async initialize(): Promise<Status> {
        const [status, timestamp] = await this.remote.get_time();
        if (status.is_ok() && timestamp) {
            this.apply_observation(timestamp);
        } else if (!status.is_ok()) {
            this.logger.error(
                `SystemClock '${this.name}' failed to get initial time: ${status.what()}`,
            );
        }
        this.monitoring_task = this.monitoring();
        return Status.ok();
    }

    async stop(): Promise<Status> {
        if (this.stop_task) {
            return this.stop_task;
        }
        this.stopped = true;
        this.stop_task = this.run_stop();
        return this.stop_task;
    }

    private async run_stop(): Promise<Status> {
        await this.remote.terminate();
        if (this.monitoring_task) {
            await this.monitoring_task;
            this.monitoring_task = undefined;
        }
        return Status.ok();
    }

    private async monitoring() {
        const retry_timeout = new RetryTimeout(MONITOR_RETRY_MS);

        while (!this.stopped) {
            const status = await this.remote.monitoring(
                MONITOR_INTERVAL_MS,
                this.handle_update.bind(this),
            );
            if (!this.stopped && !status.is_ok()) {
                this.logger.error(
                    `SystemClock '${this.name}' monitoring failed: ${status.what()}`,
                );
                await retry_timeout.wait_to_retry(() => this.stopped);
            } else {
                retry_timeout.reset();
            }
        }
    }

    private async handle_update(timestamp: midlevel.ServerTimestamp | undefined)
    : Promise<boolean>
    {
        if (timestamp !== undefined) {
            this.apply_observation(timestamp);
        }
        return !this.stopped;
    }

    private apply_observation(timestamp: midlevel.ServerTimestamp): void {
        this.world.observe(local_now_us(), timestamp.real_us, timestamp.ingame_us);
    }
}
