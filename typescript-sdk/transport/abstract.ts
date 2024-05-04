import { Status } from "../types/status.js";

export abstract class ISocket {

    abstract send(data: Uint8Array): Promise<Status>;

    abstract receive(timeout: number): Promise<[Status, Uint8Array]>;

}
