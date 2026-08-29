
const WellKnownStatus = {
    ok: "ok",
    fail: "fail",
    timeout: "timeout",
    not_connected: "not connected",
    closed: "connection closed"
}

export class Status {
    private status_id: string;
    private error?: string;
    private nested?: Status

    static ok(): Status {
        return new Status(WellKnownStatus.ok, undefined);
    }

    static fail(details: string | undefined = undefined): Status {
        return new Status(WellKnownStatus.fail, details);
    }

    static exception(error: unknown): Status {
        return new Status(
            WellKnownStatus.fail,
            error instanceof Error ? error.message : String(error));
    }

    static timeout(details: string | undefined = undefined): Status {
        return new Status(WellKnownStatus.timeout, details);
    }

    static notConnected(details: string | undefined = undefined): Status {
        return new Status(WellKnownStatus.not_connected, details);
    }

    static closed(details: string | undefined = undefined): Status {
        return new Status(WellKnownStatus.closed, details);
    }

    private constructor(status_id: string, error: string | undefined) {
        this.status_id = status_id;
        this.error = error;
    }

    public wrap(error: string): Status {
        const fail = Status.fail(error);
        if (!this.is_ok()) {
            fail.nested = this;
            fail.status_id = this.status_id;
        }
        return fail;
    }

    public what(): string {
        if (this.is_ok()) {
            return "ok";
        }
        return [
            this.error,
            this.nested
                ? this.nested.what()
                : this.status_id != WellKnownStatus.fail
                    ? `[${this.status_id}]`
                    : undefined
        ].filter(e => e).join(": ");
    }

    public is_ok(): boolean { return this.status_id == WellKnownStatus.ok; }
    public is_timeout(): boolean {
        return this.status_id == WellKnownStatus.timeout;
    }
    public is_not_connected(): boolean {
        return this.status_id == WellKnownStatus.not_connected;
    }
    public is_closed(): boolean {
        return this.status_id == WellKnownStatus.closed;
    }
}