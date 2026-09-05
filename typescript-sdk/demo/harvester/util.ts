import type { Position } from "@spx/sdk/highlevel";

export function sleep(ms: number, signal?: AbortSignal): Promise<void> {
    return new Promise((resolve, reject) => {
        if (signal?.aborted) {
            reject(abort_error());
            return;
        }
        const timer = setTimeout(resolve, ms);
        signal?.addEventListener("abort", () => {
            clearTimeout(timer);
            reject(abort_error());
        }, { once: true });
    });
}

export function wait_abort(signal: AbortSignal): Promise<void> {
    if (signal.aborted) {
        return Promise.resolve();
    }
    return new Promise((resolve) => {
        signal.addEventListener("abort", () => resolve(), { once: true });
    });
}

export function is_abort_error(error: unknown): boolean {
    return error instanceof Error && error.name === "AbortError";
}

export function abort_error(): Error {
    const error = new Error("aborted");
    error.name = "AbortError";
    return error;
}

export function distance_to(left: Position, right: Position): number {
    return Math.hypot(
        left.point[0] - right.point[0],
        left.point[1] - right.point[1],
    );
}

export function random_int(max_inclusive: number): number {
    return Math.floor(Math.random() * (max_inclusive + 1));
}

export function almostNull(
    vector: readonly [number, number],
    delta = 0.001,
): boolean {
    return vector[0] * vector[0] + vector[1] * vector[1] < delta;
}
