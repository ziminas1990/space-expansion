import * as msg from "../Protocol_pb.js";
import { IChannel, ITerminal } from "./abstract.js";
import { Decoder } from "./decoder.js";

export { UdpSocket } from "./udp_socket.js";
export { Decoder } from "./decoder.js";
export { IChannel, ITerminal, IProxy } from "./abstract.js";
export { Endpoint } from "./endpoint.js";

type Mirroring = {
    sent: (message: msg.Message) => void,
    received: (message: msg.Message) => void
};

export class MessagesDecoder extends Decoder<msg.Message, Uint8Array> {
    constructor(mirroring: Mirroring | undefined = undefined)
    {
        if (!mirroring) {
            super(
                msg.Message.fromBinary,
                (message: msg.Message) => message.toBinary()
            );
        } else {
            super(
                (data: Uint8Array) => {
                    const encoded = msg.Message.fromBinary(data);
                    mirroring.received(encoded);
                    return encoded;
                },
                (message: msg.Message) => {
                    mirroring.sent(message);
                    return message.toBinary()
                }
            );
        }
    }
}

// Some well-known types
export type MessagesChannel = IChannel<msg.Message>;
export type MessagesTerminal = ITerminal<msg.Message>;