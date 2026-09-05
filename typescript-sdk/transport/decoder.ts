import { Status } from "#sdk/types/status.js";
import { IChannel, ITerminal } from "./abstract.js"

export class Decoder<U, D> implements ITerminal<D>, IChannel<U> {
    private downlevel?: IChannel<D>;
    private uplevel?: ITerminal<U>;

    constructor(private decoder: (msg: D) => U, private encoder: (msg: U) => D)
    {}

    attach_downloevel(channel: IChannel<D>) {
        this.downlevel = channel;
    }
    detach_downloevel() {
        this.downlevel = undefined;
    }
    attach_uplevel(terminal: ITerminal<U>) {
        this.uplevel = terminal;
    }
    detach_uplevel() {
        this.uplevel = undefined;
    }

    async send(message: U): Promise<Status> {
        if (!this.downlevel) {
            return Status.fail("not attached to binary channel");
        }
        return await this.downlevel.send(this.encoder(message));
    }

    async on_message(data: D): Promise<void> {
        if (!this.uplevel) {
            return;
        }
        return this.uplevel.on_message(this.decoder(data));
    }

    async on_closed(): Promise<void> {
        if (this.uplevel) {
            await this.uplevel.on_closed();
        }
    }

    async close(): Promise<Status> {
        if (!this.downlevel) {
            return Status.fail("not attached to binary channel");
        }
        return this.downlevel.close();
    }
}
