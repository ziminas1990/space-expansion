import { fromBinary, toBinary } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import { IChannel, ITerminal } from "./abstract.js";
import { Decoder } from "./decoder.js";

export { UdpSocket } from "./udp_socket.js";
export { Decoder } from "./decoder.js";
export { IChannel, ITerminal, IProxy } from "./abstract.js";
export { Endpoint } from "./endpoint.js";

export type Mirroring = {
    sent: (message: msg.Message) => void,
    received: (message: msg.Message) => void
};

export class MessagesDecoder extends Decoder<msg.Message, Uint8Array> {
    constructor(mirroring: Mirroring | undefined = undefined)
    {
        if (!mirroring) {
            super(
                (data: Uint8Array) => fromBinary(msg.MessageSchema, data),
                (message: msg.Message) => toBinary(msg.MessageSchema, message)
            );
        } else {
            super(
                (data: Uint8Array) => {
                    const encoded = fromBinary(msg.MessageSchema, data);
                    mirroring.received(encoded);
                    return encoded;
                },
                (message: msg.Message) => {
                    mirroring.sent(message);
                    return toBinary(msg.MessageSchema, message)
                }
            );
        }
    }
}

// Some well-known types
export type MessagesChannel = IChannel<msg.Message>;
export type MessagesTerminal = ITerminal<msg.Message>;
