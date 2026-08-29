

type Waiter<T> = {
    resolve: (value: T | undefined) => void;
    timer?: ReturnType<typeof setTimeout>;
};

export class AsyncQueue<T> {
    private items: T[] = [];
    private waiters: Waiter<T>[] = [];

    put(value: T): void {
        const waiter = this.waiters.shift();
        if (!waiter) {
            this.items.push(value);
            return;
        }
        if (waiter.timer !== undefined) {
            clearTimeout(waiter.timer);
        }
        waiter.resolve(value);
    }

    async get(timeout_ms: number = Infinity): Promise<T | undefined> {
        if (this.items.length > 0) {
            return this.items.shift();
        }
        if (timeout_ms <= 0) {
            return undefined;
        }
        return await new Promise<T | undefined>((resolve) => {
            const waiter: Waiter<T> = { resolve };
            if (timeout_ms !== Infinity) {
                waiter.timer = setTimeout(() => {
                    const index = this.waiters.indexOf(waiter);
                    if (index !== -1) {
                        this.waiters.splice(index, 1);
                        resolve(undefined);
                    }
                }, timeout_ms);
            }
            this.waiters.push(waiter);
        });
    }
}
