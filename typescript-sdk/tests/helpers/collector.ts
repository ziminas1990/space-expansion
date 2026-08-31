import { waitFor } from "./wait.js";

export class Collector<T> {
    readonly items: T[] = [];

    readonly callback = (item: T): void => {
        this.items.push(item);
    };

    get length(): number {
        return this.items.length;
    }

    push(item: T): void {
        this.items.push(item);
    }

    clear(): void {
        this.items.length = 0;
    }

    take(): T | undefined {
        return this.items.shift();
    }

    async waitFor(
        predicate: (items: readonly T[]) => boolean,
        description = "collected items",
        timeoutMs = 2_000,
    ): Promise<void> {
        await waitFor(() => predicate(this.items), description, timeoutMs);
    }

    async waitForCount(
        count: number,
        description = `${count} collected items`,
        timeoutMs = 2_000,
    ): Promise<T[]> {
        await this.waitFor((items) => items.length >= count, description, timeoutMs);
        return this.items.slice(0, count);
    }
}

type HasOn<E extends PropertyKey, T> = {
    on(event: E, listener: (value: T) => unknown): unknown;
};

export function collectEvent<E extends PropertyKey, T>(
    emitter: HasOn<E, T>,
    event: E,
): Collector<T> {
    const collector = new Collector<T>();
    emitter.on(event, (value: T) => {
        collector.push(value);
    });
    return collector;
}
