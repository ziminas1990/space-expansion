import assert from "node:assert/strict";
import { Status } from "../../types/status.js";

export function expectStatus(status: Status, description?: string): void {
    assert.ok(
        status.is_ok(),
        description === undefined ? status.what() : `${description}: ${status.what()}`,
    );
}

export function expectOk<T>(
    result: [Status, T | undefined],
    description?: string,
): T {
    const [status, value] = result;
    expectStatus(status, description);
    assert.notEqual(
        value,
        undefined,
        description ?? "expected a value with an ok status",
    );
    return value as T;
}
