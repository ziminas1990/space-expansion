import * as space from "space";
import express from 'express';

import path from 'path';
import { fileURLToPath } from 'url';
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

console.log(__filename);

// Create a new express application instance
const app: express.Application = express();
app.use("/static", express.static("static"));
// Middleware to parse JSON bodies
app.use(express.json());
// Middleware to parse URL-encoded bodies
app.use(express.urlencoded({ extended: true }));

app.get("/", async (_, res) => {
    //Rerout to login page
    res.redirect("/login");
});

app.get("/login", async (_, res) => {
    res.sendFile(__dirname + "/templates/login.html");
});

app.post("/login", async (req, res) => {
    const { login, password, ip } = req.body;
    const [status, access] = await login_to_server(ip, login, password);
    if (status.isOk()) {
        res.send(`Access granted: ${JSON.stringify(access)}`);
        console.log();
    } else {
        res.send(`Access denied: ${status.what()}`);
    }
});

// Server setup
const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Server is running on http://localhost:${PORT}`);
});

async function login_to_server(ip: string, user: string, password: string) {
    const socket = new space.transport.UdpSocket();
    socket.connect(ip, 6842);

    const channel = new space.transport.MessagesChannel();
    channel.bind(socket);

    const panel = new space.lowlevel.AccessPanel(channel);
    return await panel.login(user, password);
}