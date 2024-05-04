import * as msg from "../Protocol_pb.js"
import { Status } from "../types/status.js";
import { ISocket } from "./abstract.js"

export class MessagesChannel {
    socket?: ISocket

    bind(socket: ISocket) {
        this.socket = socket;
    }

    async send(message: msg.Message): Promise<Status> {
        if (!this.socket) {
            return Status.notConnected();
        }
        return await this.socket.send(message.toBinary());
    }

    async receive(timeout: number): Promise<[Status, msg.Message | undefined]> {
        if (!this.socket) {
            return Promise.resolve([Status.notConnected(), undefined]);
        }
        const [status, data] = await this.socket.receive(timeout);
        if (!status.isOk() || !data) {
            return [status, undefined];
        }
        return [Status.ok(), msg.Message.fromBinary(data)];
    }

}