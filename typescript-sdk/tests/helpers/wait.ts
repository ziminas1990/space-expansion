
export async function waitFor(
    predicate: () => boolean | Promise<boolean>,
    description = "condition",
    timeoutMs = 2_000,
): Promise<void> {
    const stopAt = Date.now() + timeoutMs;
    while (Date.now() < stopAt) {
        if (await predicate()) {
            return;
        }
        await new Promise<void>((resolve) => setTimeout(resolve, 10));
    }
    throw new Error(`Timed out waiting for ${description}`);
}
