import type { MidlevelModule, ModuleType } from "#sdk/midlevel/module_types.js";
import { Status } from "#sdk/types/status.js";

export interface BaseModule {
    readonly type: ModuleType;
    readonly name: string;
    release(): Promise<Status>;
    reinit(rpc: MidlevelModule): Promise<Status>;
}
