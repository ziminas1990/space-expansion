import * as midlevel from "@spx/sdk/midlevel";
import { Status } from "@spx/sdk/types";
import { IWorld } from "./interfaces.js";
import { Logger } from "../log.js";
import { PassiveScanner } from "./passive_scanner.js";
import { RetryTimeout } from "../utils/retry_timeout.js";

const MONITOR_RETRY_MS = [500, 1000, 2000, 5000];

export class Ship {

    private stopped: boolean = false;
    private modules_monitoring_task?: Promise<void>;

    private readonly modules: Map<number, midlevel.ModuleInfo> = new Map();
    private readonly passive_scanners: Map<number, PassiveScanner> = new Map();

    constructor(
        private readonly remote: midlevel.Ship,
        private readonly world: IWorld,
        private readonly logger: Logger,
        private readonly name: string,
    ) {}

    async initialize(): Promise<Status> {
        const [status, modules] =
            await this.remote.commutator().get_all_modules_info();
        if (!status.is_ok()) {
            await this.remote.terminate();
            return status.wrap("Failed to get ship modules");
        }

        for (const module of modules!) {
            const module_status = await this.on_module_attached(module);
            if (!module_status.is_ok()) {
                this.logger.error(
                    `Failed to attach module '${module.module_name}' `
                    + `to ship '${this.name}': ${module_status.what()}`,
                );
            }
        }

        this.modules_monitoring_task = this.monitor_modules();
        return Status.ok();
    }

    async stop(): Promise<Status> {
        this.stopped = true;
        for (const scanner of this.passive_scanners.values()) {
            await scanner.stop();
        }
        this.passive_scanners.clear();
        this.modules.clear();
        await this.remote.terminate();
        if (this.modules_monitoring_task) {
            await this.modules_monitoring_task;
            this.modules_monitoring_task = undefined;
        }
        return Status.ok();
    }

    private async monitor_modules() {
        const retry_timeout = new RetryTimeout(MONITOR_RETRY_MS);
        while (!this.stopped) {
            const status = await this.remote.commutator().monitoring(
                this.handle_module_update.bind(this),
            );
            if (!this.stopped && !status.is_ok()) {
                this.logger.error(
                    `Ship '${this.name}' modules monitoring failed: ${status.what()}`,
                );
                await retry_timeout.wait_to_retry(() => this.stopped);
            } else {
                retry_timeout.reset();
            }
        }
    }

    private async handle_module_update(
        update: midlevel.CommutatorUpdate | undefined,
    ): Promise<boolean>
    {
        if (update?.module_attached) {
            const status = await this.on_module_attached(update.module_attached);
            if (!status.is_ok()) {
                this.logger.error(
                    `Failed to attach module '${update.module_attached.module_name}' `
                    + `to ship '${this.name}': ${status.what()}`,
                );
            }
        }
        if (update?.module_detached !== undefined) {
            await this.on_module_detached(update.module_detached);
        }
        return !this.stopped;
    }

    private async on_module_attached(info: midlevel.ModuleInfo): Promise<Status> {
        const current = this.modules.get(info.slot_id);
        if (current) {
            await this.on_module_detached(info.slot_id);
        }
        this.modules.set(info.slot_id, info);

        if (info.module_type !== midlevel.ModuleType.PASSIVE_SCANNER) {
            return Status.ok();
        }

        const scanner = new PassiveScanner(
            new midlevel.PassiveScanner(info.open_session_cb),
            this.world,
            this.logger.child(info.module_name),
            info.module_name,
        );
        const status = await scanner.initialize();
        if (!status.is_ok()) {
            this.modules.delete(info.slot_id);
            return status.wrap("Failed to initialize passive scanner");
        }
        this.passive_scanners.set(info.slot_id, scanner);
        return Status.ok();
    }

    private async on_module_detached(slot_id: number): Promise<void> {
        const info = this.modules.get(slot_id);
        if (!info) {
            return;
        }
        this.modules.delete(slot_id);

        if (info.module_type === midlevel.ModuleType.PASSIVE_SCANNER) {
            const scanner = this.passive_scanners.get(slot_id);
            if (scanner) {
                await scanner.stop();
                this.passive_scanners.delete(slot_id);
            }
        }
    }
}
