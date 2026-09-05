import * as space from "@spx/sdk";
import { toJsonString } from "@bufbuild/protobuf";
import * as api from "../common/api.js";

import express from 'express';
import session from 'express-session';
import crypto from 'crypto';
import http from 'http';
import { WebSocketServer, WebSocket } from "ws";
import path from 'path';
import { fileURLToPath } from 'url';
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

console.log(__filename);

const secret_key = "kfjdflwk45i3lrkgw3l4kgsl";

const http_port = 3000;

// Create a new express application instance
const app: express.Application = express();
const server = http.createServer(app);
const wss = new WebSocketServer({ server });

wss.on('connection', on_websocket_commection);

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
    private static all: Map<string, UserSession> = new Map();
    public static get(token: string): UserSession | undefined {
        return UserSession.all.get(token);
    }

    public token: string = crypto.randomBytes(16).toString("hex");

    constructor(
        public login: string,
        public player: space.Player)
    {
        UserSession.all.set(this.token, this);
    }
};

type CustomSession = session.Session & { token?: string };

const mirror = {
    sent: (message: space.msg.Message) => {
        if (message.choice.case == "session") {
            if (message.choice.value.choice.case == "heartbeat") {
                // do not print hearbeat messages
                return;
            }
        }
        console.log("Sent: ", toJsonString(space.msg.MessageSchema, message));
    },
    received: (message: space.msg.Message) => {
        if (message.choice.case == "session") {
            if (message.choice.value.choice.case == "heartbeat") {
                // do not print hearbeat messages
                return;
            }
        }
        console.log("Received: ", toJsonString(space.msg.MessageSchema, message));
    }
};

function on_websocket_commection(ws: WebSocket, request: http.IncomingMessage) {
    ws.on('message', function incoming(message) {
        console.log('received: %s', message);
    });

    if (request.url == undefined) {
        ws.close();
        return;
    }

    const [what, username, token] = request.url.split("/").filter((x) => x.length > 0);

    if (what != "ws" || username == undefined || token == undefined) {
        ws.close();
        return;
    }

    const user = UserSession.get(token);
    if (user == undefined || user.token != token || user.login != username) {
        ws.close();
        return;
    }

    ws.send(JSON.stringify({
        ts: Date.now(),
        items: [
            {
                type: api.ItemType.ASTEROID,
                id: 1,
                ts: Date.now(),
                pos: [100, 100, 10, 10],
                radius: 20
            } as api.Item
        ]
    }));
}

app.get("/", async (req, res) => {
    const session = req.session as CustomSession;
    if (!session.id || !session.token) {
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
    const [status, root] = await space.login(ip, login, password, mirror);
    if (status.is_ok() && root) {
        const user = new UserSession(login, root);
        (req.session as CustomSession).token = user.token;
        res.redirect("/");
    } else {
        res.redirect("/login");
    }
});

app.get("/token", async (req, res) => {
    const session = req.session as CustomSession;
    if (!session.id || !session.token) {
        res.redirect("/login");
        return;
    }

    const user_session = UserSession.get(session.token);
    if (!user_session) {
        console.error("No user session found for token: ", session.token);
        res.redirect("/login");
        return;
    }

    res.send(JSON.stringify({
        "user": user_session.login,
        "token": user_session.token
    }))
});
