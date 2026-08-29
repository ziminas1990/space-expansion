

// Value cache with a local TTL. Pass `Infinity` to never expire
// (e.g. module specification).
export class Cached<T> {
    private value: T | undefined = undefined;
    private cached_at_ms: number | undefined = undefined;
    private present = false;

    get(expiration_ms: number): T | undefined {
        if (!this.present || this.cached_at_ms === undefined) {
            return undefined;
        }
        if (expiration_ms !== Infinity &&
            performance.now() - this.cached_at_ms > expiration_ms)
        {
            return undefined;
        }
        return this.value;
    }

    set(value: T): void {
        this.value = value;
        this.cached_at_ms = performance.now();
        this.present = true;
    }

    reset(): void {
        this.value = undefined;
        this.cached_at_ms = undefined;
        this.present = false;
    }
}
