

export class RetryTimeout {
    private fail_index: number = 0;

    constructor(private timeouts_ms: number[]) {
        this.timeouts_ms.sort((a, b) => a - b);
        if (this.timeouts_ms.length === 0) {
            throw new Error("timeouts_ms is empty");
        }
        if (this.timeouts_ms[0]! <= 0) {
            throw new Error("timeouts can not be zero or negative");
        }
    }

    async wait_to_retry(should_stop?: () => boolean): Promise<void> {
        const last = this.timeouts_ms.length - 1;
        if (last < 0) {
            return;
        }
        const timeout = this.timeouts_ms[Math.min(this.fail_index, last)];
        this.fail_index = Math.min(this.fail_index + 1, last);
        if (timeout === undefined) {
            return;
        }
        const deadline = Date.now() + timeout;
        while (Date.now() < deadline) {
            if (should_stop?.()) {
                return;
            }
            const remaining = deadline - Date.now();
            await new Promise((resolve) => {
                setTimeout(resolve, Math.min(50, remaining));
            });
        }
    }

    reset(): void {
        this.fail_index = 0;
    }
}
