import * as transport from "../transport/index.js"
import { Status } from "../types/status.js";
import { AccessPanel } from "./access_panel.js"
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