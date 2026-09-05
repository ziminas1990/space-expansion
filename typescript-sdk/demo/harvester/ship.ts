import {
    ModuleType,
    type Ship as RemoteShip,
    type SystemClock,
} from "@spx/sdk/highlevel";
import { find_most_ranged_scanner, has_modules } from "./equipment.js";
import { Navigator } from "./navigator.js";
import type { World } from "./world.js";

export class Ship {
    readonly remote: RemoteShip;
    readonly world: World;
    readonly system_clock: SystemClock;
    readonly name: string;
    readonly navigator: Navigator;

    constructor(
        remote: RemoteShip,
        the_world: World,
        system_clock: SystemClock,
    ) {
        this.remote = remote;
        this.world = the_world;
        this.system_clock = system_clock;
        this.name = remote.name;
        this.navigator = new Navigator(
            `${remote.name}.navigator`,
            remote,
            system_clock,
        );
    }

    has_module(module_type: ModuleType): boolean {
        return this.remote.get_all(module_type).length > 0;
    }

    has_modules(module_types: ModuleType[]): boolean {
        return has_modules(this.remote, module_types);
    }

    async start_passive_scanning(): Promise<void> {
        const scanner = await find_most_ranged_scanner(this.remote);
        if (!scanner) {
            return;
        }
        this.world.update_objects(scanner.objects());
        scanner.on("update", (object) => {
            this.world.update_object(object);
        });
        scanner.start_monitoring();
    }

    stop(): void {
        this.navigator.interrupt();
    }
}
