import * as space from "space";
import express from 'express';
import session from 'express-session';

import path from 'path';
import { fileURLToPath } from 'url';
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

console.log(__filename);

const secret_key = "kfjdflwk45i3lrkgw3l4kgsl";

// Create a new express application instance
const app: express.Application = express();
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
    constructor(public port: number, public root_session_id: number) {}
};

type CustomSession = session.Session & { user?: UserSession };


app.get("/", async (req, res) => {
    const session = req.session as CustomSession;
    if (!session.id || !session.user) {
        res.redirect("/login");
        return;
    }

    const user = session.user;
    res.send(`Session #${user.root_session_id} on port: ${user.port}`);
});

app.get("/login", async (_, res) => {
    res.sendFile(__dirname + "/templates/login.html");
});

app.post("/login", async (req, res) => {
    const { login, password, ip } = req.body;
    const [status, access] = await login_to_server(ip, login, password);
    if (status.isOk()) {
        (req.session as CustomSession).user = new UserSession(
            access?.port ?? 0,
            access?.session_id ?? 0
        );
        res.redirect("/");
    } else {
        res.redirect("/login");
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