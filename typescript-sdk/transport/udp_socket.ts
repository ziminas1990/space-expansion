import dgram from "node:dgram"
import { Status } from "../types/status.js";
import { IChannel, ITerminal } from "./abstract.js";


export class UdpSocket extends IChannel<Uint8Array> {
    private socket: dgram.Socket;
    private remote? : { address: string, port: number };
    private uplevel?: ITerminal<Uint8Array>;

    constructor() {
        super();
        this.socket = dgram.createSocket("udp4");
        this.socket.unref();

        this.socket.on("message", async (msg, rinfo) => {
            if (rinfo.address != this.remote?.address ||
                rinfo.port != this.remote?.port) {
                return;
            }
            if (this.uplevel) {
                await this.uplevel.on_message(msg);
            }
        });

        this.socket.on("error", (err) => {
            console.error("Socket error", err);
        });

        this.socket.on("close", async () => {
            if (this.uplevel) {
                await this.uplevel.on_closed();
            }
        });
    }

    connect(address: string, port: number): Status {
        this.remote = { address, port };
        return Status.ok();
    }

    attach(terminal: ITerminal<Uint8Array>) {
        this.uplevel = terminal;
    }

    detach() {
        this.uplevel = undefined;
    }

    async send(data: Uint8Array): Promise<Status> {
        if (!this.remote) {
            return Status.notConnected("remote address is not set");
        }

        return new Promise<Status>((resolve, reject) => {
            this.socket.send(data, this.remote!.port, this.remote!.address,
                (err) => {
                    if (err) {
                        reject(Status.fail(err.message));
                    } else {
                        resolve(Status.ok());
                    }
                }
            );
        });
    }

    async close(): Promise<Status> {
        this.socket.close();
        return Status.ok();
    }
}