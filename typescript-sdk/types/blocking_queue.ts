
export class BlockingQueue<T> {
    private queue: T[] = [];
    private waiters: { resolve: (value: T | PromiseLike<T>) => void }[] = [];

    enqueue(item: T): void {
        if (this.waiters.length > 0) {
            const waiter = this.waiters.shift();
            if (waiter) {
                waiter.resolve(item);
                return;
            }
        }
        this.queue.push(item);
    }

    async dequeue(timeoutMs: number = 0): Promise<T | undefined> {
        if (this.queue.length > 0) {
            return this.queue.shift();
        } else {
            if (timeoutMs == 0) {
                return new Promise<T>((resolve) => {
                    this.waiters.push({ resolve });
                });
            } else {
                return new Promise<T | undefined>((resolve) => {
                    const timer = setTimeout(() => {
                        clearTimeout(timer);
                        const index = this.waiters.findIndex(waiter => waiter.resolve == resolve);
                        if (index != -1) {
                            this.waiters.splice(index, 1);
                        }
                        resolve(undefined);
                    }, timeoutMs);

                    this.waiters.push({
                        resolve: (item) => {
                            clearTimeout(timer);
                            resolve(item);
                        }
                    });
                });
            }
        }
    }
}
