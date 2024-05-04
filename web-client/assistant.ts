// import { UdpSocket } from "space-axpansion/transport/udp_socket.js";
// import { AccessPanel } from "./lowlevel/access_panel.js";
// import { MessagesChannel } from "./transport/channels.js";

import * as space from "space";

async function main() {
    const socket = new space.transport.UdpSocket();
    socket.connect("127.0.0.1", 6842);

    const channel = new space.transport.MessagesChannel();
    channel.bind(socket);

    const panel = new space.lowlevel.AccessPanel(channel);
    const [status, access] = await panel.login("Olenoid", "admin");
    if (status.isOk()) {
        console.log(`Access granted: ${JSON.stringify(access)}`);
    } else {
        console.error(`Access denied: ${status.what()}`);
    }
}

main();