import { create } from "@bufbuild/protobuf";
import * as admin from "../Privileged_pb.js";
import * as transport from "../transport/index.js"
import { Status } from "../types/status.js";
import { AccessPanel } from "./access_panel.js"
import { Administrator, privilegedDecoder } from "./administrator.js";
import { RootSession } from "./root_session.js";

export async function login(
    ip: string,
    user: string,
    password: string,
    mirroring: transport.Mirroring | undefined = undefined,
    port: number = 6842,
): Promise<[Status,  RootSession | undefined]>
{
    const socket = new transport.UdpSocket();
    socket.connect(ip, port);

    const decoder = new transport.MessagesDecoder(mirroring);
    decoder.attach_downloevel(socket);
    socket.attach(decoder);

    const panel = new AccessPanel(decoder);
    decoder.attach_uplevel(panel);

    const [status, access] = await panel.login(user, password);
    if (!status.is_ok() || !access) {
        return [status.wrap("login failed"), undefined];
    }
    decoder.detach_uplevel();

    // Redirect socket to the new port
    socket.connect(ip, access.port);

    const root_session = new RootSession(decoder, access.session_id);
    decoder.attach_uplevel(root_session);
    return [Status.ok(), root_session];
}

export async function login_as_administrator(
    ip: string,
    port: number,
    login: string,
    password: string,
    timeout_ms: number = 500,
): Promise<[Status, Administrator | undefined]> {
    const socket = new transport.UdpSocket();
    socket.connect(ip, port);

    const decoder = privilegedDecoder();
    decoder.attach_downloevel(socket);
    socket.attach(decoder);

    const access = new transport.Endpoint<admin.Message>();
    decoder.attach_uplevel(access);

    const request = create(admin.MessageSchema, {
        choice: {
            case: "access",
            value: create(admin.AccessSchema, {
                choice: {
                    case: "login",
                    value: create(admin.Access_LoginSchema, { login, password }),
                },
            }),
        },
    });

    const send_status = await decoder.send(request);
    if (!send_status.is_ok()) {
        await decoder.close();
        return [send_status.wrap("failed to send administrator login"), undefined];
    }

    const [receive_status, response] = await access.wait(timeout_ms);
    decoder.detach_uplevel();
    if (!receive_status.is_ok() || response === undefined) {
        await decoder.close();
        return [
            receive_status.wrap("no administrator login response"),
            undefined,
        ];
    }
    if (
        response.choice.case !== "access" ||
        response.choice.value.choice.case !== "success"
    ) {
        await decoder.close();
        return [Status.fail("administrator login was rejected"), undefined];
    }

    const administrator = new Administrator(
        decoder,
        response.choice.value.choice.value,
    );
    decoder.attach_uplevel(administrator);
    return [Status.ok(), administrator];
}