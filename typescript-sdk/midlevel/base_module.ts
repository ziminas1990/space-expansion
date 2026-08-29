import { Status } from "../types/status.js";
import * as lowlevel from "../lowlevel/index.js";

export type OpenSessionCallback = () => Promise<[Status, lowlevel.Session | undefined]>;
export type CreateLowlevelInterface<I> = (session: lowlevel.Session) => Promise<[Status, I | undefined]>;
export type UserLogicCallback<I, T> = (session: I) => Promise<[Status, T]>;

export class BaseModule<I> {
    // Sessions that can be reused for communication
    private sessions: lowlevel.Session[] = [];
    // Sessions currently used by run() (including dedicated long-running ones)
    private in_use: Set<lowlevel.Session> = new Set();
    private active: boolean = true;

    constructor(
        protected readonly open_session_cb: OpenSessionCallback,
        private create_lowlevel_interface: CreateLowlevelInterface<I>
    ) {}

    async terminate() {
        this.active = false;
        const pooled = this.sessions;
        this.sessions = [];
        const busy = Array.from(this.in_use);
        this.in_use.clear();
        for (const session of [...pooled, ...busy]) {
            await session.close();
        }
    }

    // Run the specified callback in a session. If 'close_session' is true,
    // session will be closed after the callback is executed, otherwise it
    // will be held for future reuse.
    async run<T>(callback: UserLogicCallback<I, T>,
                 close_session: boolean = false): Promise<[Status, T | undefined]>
    {
        if (!this.active) {
            return [Status.closed("terminated"), undefined];
        }
        const [status, session] = await this.get_session();
        if (!status.is_ok() || !session) {
            return [status.wrap("no available session"), undefined];
        }

        const [if_status, iface] = await this.create_lowlevel_interface(session);
        if (!if_status.is_ok() || !iface) {
            await session.close();
            return [if_status.wrap("failed to create lowlevel interface"), undefined];
        }

        this.in_use.add(session);
        const [user_status, result] = await callback(iface);
        this.in_use.delete(session);
        if (user_status.is_ok() && !close_session && this.active) {
            this.sessions.push(session);
        } else {
            await session.close();
        }
        return [user_status, result];
    }

    // The same as "run", but the callback doesn't return anything, hence
    // the result is just a status.
    async run_no_return(callback: (session: I) => Promise<Status>,
                        close_session: boolean = false): Promise<Status>
    {
        const [status, _] = await this.run(async (session) => {
            return [await callback(session), undefined];
        }, close_session);
        return status;
    }

    private async get_session(): Promise<[Status, lowlevel.Session | undefined]> {
        if (!this.active) {
            return [Status.closed("terminated"), undefined];
        }
        while (this.sessions.length > 0) {
            const candidate = this.sessions.pop()!;
            if (candidate.is_active()) {
                return [Status.ok(), candidate];
            }
        }
        let last_status = Status.ok();
        for (let attempt = 0; attempt < 3; attempt++) {
            if (!this.active) {
                return [Status.closed("terminated"), undefined];
            }
            const [status, session] = await this.open_session_cb();
            if (status.is_ok() && session) {
                return [Status.ok(), session];
            }
            last_status = status;
        }
        return [last_status.wrap("failed to open new session"), undefined];
    }

}
