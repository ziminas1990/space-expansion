import type { SystemClock } from "../../../highlevel/index.js";
import { create_logger } from "../log.js";
import { is_abort_error } from "../util.js";

export type JournalRecord = {
    timestamp: bigint;
    action: string;
};

export abstract class BaseTask {
    static global_id = 0;

    readonly id: number;
    readonly name: string;
    readonly journal: JournalRecord[] = [];
    finished = false;

    protected readonly system_clock: SystemClock;
    protected readonly write_logs: boolean;
    private readonly _logger;
    private _abort?: AbortController;
    private _running?: Promise<boolean>;
    private _complete_cb?: (status: boolean) => void;

    constructor(name: string, system_clock: SystemClock, write_logs = true) {
        BaseTask.global_id += 1;
        this.id = BaseTask.global_id;
        this.name = name;
        this.system_clock = system_clock;
        this.write_logs = write_logs;
        this._logger = create_logger(`${name}:${this.id}`);
    }

    protected get signal(): AbortSignal | undefined {
        return this._abort?.signal;
    }

    add_journal_record(action: string): void {
        const record: JournalRecord = {
            timestamp: this.system_clock.now_us(),
            action,
        };
        this.journal.push(record);
        if (this.write_logs) {
            this._logger.info(`${record.timestamp}: ${record.action}`);
        }
    }

    async run(): Promise<boolean> {
        this.finished = false;
        this._abort = new AbortController();
        this.add_journal_record("Started");
        let status = false;
        try {
            status = await this._impl();
            this.add_journal_record(status ? "Finished" : "Failed");
        } catch (error) {
            if (is_abort_error(error) || this._abort.signal.aborted) {
                this.add_journal_record("Canceled");
            } else {
                throw error;
            }
        }
        this.finished = true;
        this._complete_cb?.(status);
        return status;
    }

    run_async(complete_cb?: (status: boolean) => void): void {
        this._complete_cb = complete_cb;
        this._running = this.run();
    }

    async join(): Promise<boolean | undefined> {
        return this._running;
    }

    interrupt(): void {
        if (this._abort === undefined) {
            return;
        }
        if (!this._abort.signal.aborted) {
            this.add_journal_record("Interrupted");
            this._abort.abort();
        }
    }

    protected abstract _impl(): Promise<boolean>;
}
