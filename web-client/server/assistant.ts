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
    private static all: Map<string, UserSession> = new Map();
    public static get(token: string): UserSession | undefined {
        return UserSession.all.get(token);
    }

    public token: string = crypto.randomBytes(16).toString("hex");

    constructor(
        public login: string,
        public root_session: space.lowlevel.RootSession)
    {
        UserSession.all.set(this.token, this);
    }
};

type CustomSession = session.Session & { token?: string };


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

const mirror = {
    sent: (message: space.msg.Message) => {
        console.log("Sent: ", message.toJsonString());
    },
    received: (message: space.msg.Message) => {
        console.log("Received: ", message.toJsonString());
    }
};

app.post("/login", async (req, res) => {
    const { login, password, ip } = req.body;
    const [status, root] = await space.lowlevel.login(ip, login, password, mirror);
    if (status.isOk() && root) {
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

    console.log("Token: ", session.token);
    res.send(JSON.stringify({
        "user": user_session.login,
        "token": user_session.token
    }))
});
