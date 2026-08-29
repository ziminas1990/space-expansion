import type { MidlevelModule, ModuleType } from "../midlevel/module_types.js";
import { Status } from "../types/status.js";

export interface BaseModule {
    readonly type: ModuleType;
    readonly name: string;
    release(): Promise<Status>;
    reinit(rpc: MidlevelModule): Promise<Status>;
}
