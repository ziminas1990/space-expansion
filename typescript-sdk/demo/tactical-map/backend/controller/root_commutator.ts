import * as midlevel from "@spx/sdk/midlevel";
import { Status } from "@spx/sdk/types";
import { IWorld } from "./interfaces.js";
import { Logger } from "../log.js";
import { Ship } from "./ship.js";
import { SystemClock } from "./system_clock.js";

enum RootState {
    OFFLINE = "OFFLINE",
    INITIALIZING = "INITIALIZING",
    ONLINE = "ONLINE",
    DISCONNECTING = "DISCONNECTING",
}

export class RootCommutator {

    // slot_id -> module_info
    private modules: Map<number, midlevel.ModuleInfo> = new Map();
    // slot_id -> ship controller
    private ships: Map<number, Ship> = new Map();
    private system_clock?: { slot_id: number, controller: SystemClock };

    private state: RootState = RootState.OFFLINE;
    private monitoring_task: Promise<void>;

    constructor(
        private readonly commutator: midlevel.Commutator,
        private readonly world: IWorld,
        private readonly logger: Logger,
        private readonly on_unsolicited_stop?: () => void,
    ) {
        this.monitoring_task = Promise.resolve();
    }

    async initialize(): Promise<Status> {
        if (this.state !== RootState.OFFLINE) {
            return Status.fail(
                `Cannot initialize root commutator from ${this.state}`,
            );
        }
        this.state = RootState.INITIALIZING;

        const [status, modules] = await this.commutator.get_all_modules_info();
        if (!status.is_ok()) {
            this.state = RootState.OFFLINE;
            return status.wrap("Failed to get all modules info");
        }
        for (const module of modules!) {
            const attached_status = await this.on_module_attached(module);
            if (!attached_status.is_ok()) {
                this.logger.error(
                    `Failed to handle module attached: ${attached_status.what()}`,
                );
            }
        }
        this.monitoring_task = this.monitoring();
        this.state = RootState.ONLINE;
        return Status.ok();
    }

    verify(): Status {
        if (this.state !== RootState.ONLINE) {
            return Status.fail("Root commutator is not initialized");
        }
        if (!this.system_clock) {
            return Status.fail("SystemClock is not running");
        }
        return Status.ok();
    }

    async stop(reason: string): Promise<Status> {
        if (this.state !== RootState.ONLINE) {
            return Status.fail(`Cannot stop root commutator from ${this.state}`);
        }
        this.logger.info(`Stopping root commutator: ${reason}`);
        this.state = RootState.DISCONNECTING;

        const monitoring = this.monitoring_task;
        await this.stop_children();
        await this.commutator.terminate();
        await monitoring;

        this.state = RootState.OFFLINE;
        return Status.ok();
    }

    is_stopped(): boolean {
        return this.state === RootState.OFFLINE;
    }

    private async stop_children(): Promise<void> {
        while (this.ships.size > 0 || this.system_clock) {
            const ships = [...this.ships.values()];
            this.ships.clear();
            const clock = this.system_clock;
            this.system_clock = undefined;
            await Promise.all([
                ...ships.map((ship) => ship.stop()),
                clock?.controller.stop() ?? Promise.resolve(),
            ]);
        }
    }

    private async monitoring() {
        const status = await this.commutator.monitoring(this.handle_update.bind(this));
        if (!status.is_ok()) {
            this.logger.error(`Monitoring error: ${status.what()}`);
        }
        this.logger.info("Monitoring finished");
        this.monitoring_task = Promise.resolve();
        if (this.state !== RootState.ONLINE) {
            return;
        }
        this.on_unsolicited_stop?.();
        if (this.state === RootState.ONLINE) {
            void this.stop("monitoring finished");
        }
    }

    private async handle_update(update: midlevel.CommutatorUpdate | undefined)
    : Promise<boolean> {
        if (update === undefined) {
            return this.state === RootState.ONLINE;
        }
        if (update.module_attached) {
            await this.on_module_attached(update.module_attached);
        }
        if (update.module_detached !== undefined) {
            await this.on_module_detached(update.module_detached);
        }
        return this.state === RootState.ONLINE;
    }

    private async on_module_attached(module_info: midlevel.ModuleInfo)
    : Promise<Status>
    {
        if (this.state !== RootState.ONLINE && this.state !== RootState.INITIALIZING) {
            return Status.fail(`Root commutator is ${this.state}`);
        }
        this.modules.set(module_info.slot_id, module_info);
        if (module_info.module_type === midlevel.ModuleType.SHIP) {
            return this.on_new_ship(module_info);
        }
        if (module_info.module_type === midlevel.ModuleType.SYSTEM_CLOCK) {
            return this.on_new_system_clock(module_info);
        }
        return Status.ok();
    }

    private async on_module_detached(slot_id: number): Promise<Status> {
        const module_info = this.modules.get(slot_id);
        if (!module_info) {
            return Status.ok();
        }
        this.modules.delete(slot_id);
        if (module_info.module_type === midlevel.ModuleType.SHIP) {
            return this.on_ship_detached(module_info);
        }
        if (module_info.module_type === midlevel.ModuleType.SYSTEM_CLOCK) {
            return this.on_system_clock_detached(module_info);
        }
        return Status.ok();
    }

    private async on_new_ship(info: midlevel.ModuleInfo): Promise<Status> {
        if (this.state !== RootState.ONLINE && this.state !== RootState.INITIALIZING) {
            return Status.fail(`Root commutator is ${this.state}`);
        }
        if (this.ships.has(info.slot_id)) {
            return Status.ok();
        }

        const ship = new Ship(
            new midlevel.Ship(info.open_session_cb),
            this.world,
            this.logger.child(info.module_name),
            info.module_name,
        );
        const status = await ship.initialize();
        if (!status.is_ok()) {
            return status;
        }
        this.ships.set(info.slot_id, ship);
        if (this.state !== RootState.ONLINE && this.state !== RootState.INITIALIZING) {
            this.ships.delete(info.slot_id);
            await ship.stop();
            return Status.fail(`Root commutator is ${this.state}`);
        }
        return Status.ok();
    }

    private async on_ship_detached(info: midlevel.ModuleInfo): Promise<Status> {
        const ship = this.ships.get(info.slot_id);
        if (ship) {
            await ship.stop();
            this.ships.delete(info.slot_id);
        }
        return Status.ok();
    }

    private async on_new_system_clock(info: midlevel.ModuleInfo): Promise<Status> {
        if (this.state !== RootState.ONLINE && this.state !== RootState.INITIALIZING) {
            return Status.fail(`Root commutator is ${this.state}`);
        }
        if (this.system_clock?.slot_id === info.slot_id) {
            return Status.ok();
        }

        const clock = new SystemClock(
            new midlevel.SystemClock(info.open_session_cb),
            this.world,
            this.logger.child(info.module_name),
            info.module_name,
        );
        const status = await clock.initialize();
        if (!status.is_ok()) {
            return status;
        }

        if (this.system_clock) {
            this.logger.warning(
                `Second SystemClock '${info.module_name}' at slot ${info.slot_id}; `
                + `replacing the one at slot ${this.system_clock.slot_id}`,
            );
            await this.system_clock.controller.stop();
        }
        this.system_clock = { slot_id: info.slot_id, controller: clock };
        if (this.state !== RootState.ONLINE && this.state !== RootState.INITIALIZING) {
            this.system_clock = undefined;
            await clock.stop();
            return Status.fail(`Root commutator is ${this.state}`);
        }
        return Status.ok();
    }

    private async on_system_clock_detached(info: midlevel.ModuleInfo): Promise<Status> {
        if (this.system_clock?.slot_id === info.slot_id) {
            await this.system_clock.controller.stop();
            this.system_clock = undefined;
        }
        return Status.ok();
    }
}
