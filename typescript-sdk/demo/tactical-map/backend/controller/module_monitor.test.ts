import { expect, test } from "vitest";
import { Status } from "@spx/sdk/types";
import { noop_logger } from "../../common/logger.js";
import { ModuleMonitor } from "./module_monitor.js";

test("publishes initial and later module values, then ignores updates after stop", async () => {
    let callback: ((value: number | undefined) => Promise<boolean>) | undefined;
    let finish: ((status: Status) => void) | undefined;
    const values: number[] = [];
    const remote = {
        monitoring: async (on_value: (value: number | undefined) => Promise<boolean>) => {
            callback = on_value;
            return await new Promise<Status>((resolve) => { finish = resolve; });
        },
        terminate: async () => { finish?.(Status.ok()); },
    };
    const monitor = new ModuleMonitor(remote, noop_logger, "engine", (value) => {
        values.push(value);
    });

    monitor.start();
    expect(await callback?.(0)).toBe(true);
    expect(await callback?.(25)).toBe(true);
    expect(await callback?.(undefined)).toBe(true);
    expect(values).toEqual([0, 25]);

    await monitor.stop();
    expect(await callback?.(50)).toBe(false);
    expect(values).toEqual([0, 25]);
});
