import { Status } from "../types/status.js";

// Something that receives and handle incoming meesages of type T
export abstract class ITerminal<T> {
    abstract on_message(message: T): Promise<void>;
    abstract on_closed(): Promise<void>;
}

// Something that can be used to send messages of type T
export abstract class IChannel<T> {
    abstract send(message: T): Promise<Status>;
    abstract close(): Promise<Status>;
}

export abstract class IProxy<T> extends ITerminal<T> implements IChannel<T> {
    abstract send(message: T): Promise<Status>;
    abstract close(): Promise<Status>;
}