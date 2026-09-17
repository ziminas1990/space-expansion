import { FormEvent, useState } from "react";
import type { LoginCredentials } from "./world_client.js";

export const DEFAULT_LOGIN: LoginCredentials = {
    server: "127.0.0.1",
    port: 6842,
    login: "Olenoid",
    password: "admin",
};

type LoginFormProps = {
    disabled: boolean;
    on_submit: (credentials: LoginCredentials) => void;
};

export function LoginForm({ disabled, on_submit }: LoginFormProps) {
    const [server, set_server] = useState(DEFAULT_LOGIN.server);
    const [port, set_port] = useState(String(DEFAULT_LOGIN.port));
    const [login, set_login] = useState(DEFAULT_LOGIN.login);
    const [password, set_password] = useState(DEFAULT_LOGIN.password);
    const [local_error, set_local_error] = useState<string | undefined>();

    function handle_submit(event: FormEvent<HTMLFormElement>): void {
        event.preventDefault();
        const parsed_port = Number(port);
        if (!Number.isFinite(parsed_port) || parsed_port <= 0 || parsed_port > 65535) {
            set_local_error("Port must be a number between 1 and 65535");
            return;
        }
        set_local_error(undefined);
        on_submit({
            server: server.trim(),
            port: Math.trunc(parsed_port),
            login,
            password,
        });
    }

    return (
        <form className="login-form" onSubmit={handle_submit} autoComplete="off">
            <label>
                Server
                <input
                    name="server"
                    value={server}
                    disabled={disabled}
                    onChange={(event) => set_server(event.target.value)}
                />
            </label>
            <label>
                Port
                <input
                    name="port"
                    type="number"
                    min={1}
                    max={65535}
                    value={port}
                    disabled={disabled}
                    onChange={(event) => set_port(event.target.value)}
                />
            </label>
            <label>
                Login
                <input
                    name="login"
                    value={login}
                    disabled={disabled}
                    onChange={(event) => set_login(event.target.value)}
                />
            </label>
            <label>
                Password
                <input
                    name="password"
                    type="password"
                    value={password}
                    disabled={disabled}
                    onChange={(event) => set_password(event.target.value)}
                />
            </label>
            {local_error !== undefined && (
                <p className="status-message status-error">{local_error}</p>
            )}
            <button type="submit" disabled={disabled}>
                {disabled ? "Connecting…" : "Connect"}
            </button>
        </form>
    );
}
