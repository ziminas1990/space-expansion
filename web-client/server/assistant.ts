import * as space from "space";
import express from 'express';
import session from 'express-session';
import crypto from 'crypto';
import http from 'http';
import { WebSocketServer } from "ws";
import path from 'path';
import { fileURLToPath } from 'url';
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

import * as api from "../common/api.js";
console.log(__filename);

const secret_key = "kfjdflwk45i3lrkgw3l4kgsl";

const http_port = 3000;

// Create a new express application instance
const app: express.Application = express();
const server = http.createServer(app);
const wss = new WebSocketServer({ server });

wss.on('connection', function connection(ws) {
    ws.on('message', function incoming(message) {
        console.log('received: %s', message);
    });
    ws.send(JSON.stringify({
        ts: Date.now(),
        items: [
            {
                type: api.ItemType.ASTEROID,
                id: 1,
                ts: Date.now(),
                pos: [0, 0, 0, 0],
                radius: 10
            }
        ]
    }));
});

// Server setup
server.listen(http_port, () => {
    console.log(`Server is running on http://localhost:${http_port}`);
});

// Configure express app
app.set("view engine", "mustache");

app.use("/static", express.static("static"));
// Middleware to parse JSON bodies
app.use(express.json());
// Middleware to parse URL-encoded bodies
app.use(express.urlencoded({ extended: true }));

app.use(session({
    secret: secret_key,
    resave: false,
    saveUninitialized: true,
}));

class UserSession {
    public token: string = crypto.randomBytes(16).toString("hex");

    constructor(
        public login: string,
        public port: number,
        public root_session_id: number) {}
};

type CustomSession = session.Session & { user?: UserSession };


app.get("/", async (req, res) => {
    const session = req.session as CustomSession;
    if (!session.id || !session.user) {
        res.redirect("/login");
        return;
    }

    //const user = session.user;
    res.sendFile(__dirname + "/templates/index.html");
});

app.get("/login", async (_, res) => {
    res.sendFile(__dirname + "/templates/login.html");
});

app.post("/login", async (req, res) => {
    const { login, password, ip } = req.body;
    const [status, access] = await login_to_server(ip, login, password);
    if (status.isOk()) {
        (req.session as CustomSession).user = new UserSession(
            login,
            access?.port ?? 0,
            access?.session_id ?? 0
        );
        res.redirect("/");
    } else {
        res.redirect("/login");
    }
});

app.get("/token", async (req, res) => {
    const session = req.session as CustomSession;
    if (!session.id || !session.user) {
        res.redirect("/login");
        return;
    }

    console.log("Token: ", session.user.token);
    res.send(JSON.stringify({
        "user": session.user.login,
        "token": session.user.token
    }))
});

async function login_to_server(ip: string, user: string, password: string) {
    const socket = new space.transport.UdpSocket();
    socket.connect(ip, 6842);

    const channel = new space.transport.MessagesChannel();
    channel.bind(socket);

    const panel = new space.lowlevel.AccessPanel(channel);
    return await panel.login(user, password);
}