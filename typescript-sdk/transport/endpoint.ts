import { Status } from '../types/status.js';
import { ITerminal } from './abstract.js';

// Endpoint stores all incoming messages into an internal queue and allows to
// wait for them one by one
export class Endpoint<T> implements ITerminal<T> {
    private queue: T[] = [];
    private readers: ((status: Status, msg: T | undefined) => void)[] = [];

    async on_message(message: T): Promise<void> {
        const reader = this.readers.shift();
        if (reader) {
            reader(Status.ok(), message);
        } else {
            this.queue.push(message);
        }
    }

    async on_closed(): Promise<void> {
        this.readers.forEach(reader => {
            reader(Status.closed("channel is closed"), undefined);
        });
        this.readers = [];
    }

    async wait(timeout_ms: number = 500): Promise<[Status, T | undefined]> {
        const message = this.queue.shift();
        if (message) {
            return [Status.ok(), message];
        };

        return new Promise<[Status, T | undefined]>((resolve) => {
            const resolve_wrapper = (status: Status, msg: T | undefined) => {
                clearTimeout(timer);
                resolve([status, msg!]);
            }

            this.readers.push(resolve_wrapper);

            const timer = setTimeout(() => {
                const index = this.readers.indexOf(resolve_wrapper);
                if (index != -1) {
                    this.readers.splice(index, 1);
                }
                resolve([Status.timeout(), undefined]);
            }, timeout_ms);
        });
    }
}