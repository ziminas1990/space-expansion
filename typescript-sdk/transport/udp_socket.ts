import dgram from "node:dgram"
import { Status } from "../types/status.js";
import { ISocket } from "./abstract.js";


export class UdpSocket extends ISocket {
    private socket: dgram.Socket;
    private remote? : { address: string, port: number };

    private receive_queue: Uint8Array[] = [];
    private readers: ((status: Status, msg: Uint8Array | undefined) => void)[] = [];

    constructor() {
        super();
        this.socket = dgram.createSocket("udp4");
        this.socket.unref();

        this.socket.on("message", (msg, rinfo) => {
            if (rinfo.address != this.remote?.address ||
                rinfo.port != this.remote?.port) {
                return;
            }
            const reader = this.readers.shift();
            if (reader) {
                reader(Status.ok(), new Uint8Array(msg));
            } else {
                this.receive_queue.push(new Uint8Array(msg));
            }
        });

        this.socket.on("error", (err) => {
            this.readers.forEach((reader) => {
                reader(Status.fail(err.message), undefined);
            });
            this.readers = []
        });

        this.socket.on("close", () => {
            this.readers.forEach((reader) => {
                reader(Status.closed(), undefined);
            });
            this.readers = []
        });
    }

    connect(address: string, port: number): Status {
        this.remote = { address, port };
        return Status.ok();
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

    async receive(timeout: number): Promise<[Status, Uint8Array]> {
        if (this.receive_queue.length > 0) {
            return [Status.ok(), this.receive_queue.shift()!];
        }

        return new Promise<[Status, Uint8Array]>((resolve) => {
            const resolve_wrapper = (status: Status, msg: Uint8Array) => {
                clearTimeout(timer);
                resolve([status, msg!]);
            }

            this.readers.push(resolve_wrapper);

            const timer = setTimeout(() => {
                const index = this.readers.indexOf(resolve_wrapper);
                if (index != -1) {
                    this.readers.splice(index, 1);
                }
                resolve([Status.timeout(), new Uint8Array(0)]);
            }, timeout);
        });
    }
}