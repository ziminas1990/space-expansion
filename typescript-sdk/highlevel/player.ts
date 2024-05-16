import { Status } from "../types/status.js";
import { Commutator, ModuleInfo } from "./commutator.js";
import { Ship } from "./ship.js";
import * as midlevel  from "../midlevel/index.js";
import assert from "assert";


export class Player {

    public ships: Map<string, Ship> = new Map();

    constructor(private root_commutator: Commutator) {}

    async init(): Promise<Status> {
        this.root_commutator.on("attached", this.module_attached.bind(this));
        this.root_commutator.on("detached", this.module_detached.bind(this));

        // Start root commutator
        {
            const status = await this.root_commutator.init();
            if (!status.is_ok()) {
                return status.wrap("Failed to init root commutator");
            }
        }

        return Status.ok();
    }

    private async module_attached(module: ModuleInfo) {
        if (module.module_type.startsWith("Ship/")) {
            const status = await this.on_ship_attached(module);
            if (!status.is_ok()) {
                console.error(`Failed to attach ship ${module.module_name}: ${status.what()}`);
            }
        }
    }

    private module_detached(module: ModuleInfo) {
        if (module.module_type == "ship") {
            const ship = this.ships.get(module.module_name);
            if (ship) {
                ship.release();
                this.ships.delete(module.module_name);
            }
        }
    }

    private async on_ship_attached(module: ModuleInfo): Promise<Status>
    {
        console.log("GREPIT: on_ship_attached ", module);
        if (this.ships.has(module.module_name)) {
            const ship = this.ships.get(module.module_name);
            assert(ship);
            return ship.reinit(new midlevel.Ship(module.open_session_cb));
        }
        const ship = new Ship(
            new midlevel.Ship(module.open_session_cb),
            module.module_name);
        const status = await ship.init();
        if (!status.is_ok()) {
            console.error(`Failed to init ship ${module.module_name}: ${status}`);
            return status;
        }
        this.ships.set(module.module_name, ship);
        return Status.ok();
    }

}