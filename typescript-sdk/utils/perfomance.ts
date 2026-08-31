

export async function measure_promise<T>(
    promise: Promise<T>,
    callback: (duration: number) => void,
): Promise<T> {
    const start = performance.now();
    const result = await promise;
    const end = performance.now();
    callback(end - start);
    return result;
}

export function measure_function<T>(
    fn: () => T,
    callback: (duration: number) => void,
): T {
    const start = performance.now();
    const result = fn();
    const end = performance.now();
    callback(end - start);
    return result;
}