import { useState, useSyncExternalStore } from "react";
import { LoginForm } from "./LoginForm.js";
import { PixiMap } from "./map/PixiMap.js";
import { ShipList } from "./ShipList.js";
import {
    WorldClient,
    type ClientStatus,
    type LoginCredentials,
} from "./world_client.js";

export function App() {
    const [client] = useState(() => new WorldClient());
    const version = useSyncExternalStore(client.subscribe, client.get_version);
    const status = client.get_status();
    const world = client.get_world();
    const followed_ship_id = client.get_followed_ship_id();

    function handle_login(credentials: LoginCredentials): void {
        client.connect(credentials);
    }

    if (world === undefined) {
        return (
            <div className="app login-screen">
                <div className="login-panel">
                    <h1>Tactical Map</h1>
                    <StatusMessage status={status} />
                    <LoginForm
                        disabled={
                            status.type === "connecting" || status.type === "connected"
                        }
                        on_submit={handle_login}
                    />
                </div>
            </div>
        );
    }

    return (
        <div className="app tactical-screen" data-version={version}>
            <PixiMap
                world={world}
                version={version}
                followed_ship_id={followed_ship_id}
                on_release_follow={() => client.set_followed_ship_id(undefined)}
            />
            <div className="connection-chip">Connected</div>
            <ShipList
                world={world}
                followed_ship_id={followed_ship_id}
                on_select={(id) => client.set_followed_ship_id(id)}
            />
        </div>
    );
}

function StatusMessage({ status }: { status: ClientStatus }) {
    switch (status.type) {
        case "connecting":
            return <p className="status-message">Connecting…</p>;
        case "login_error":
            return (
                <p className="status-message status-error">{status.reason}</p>
            );
        case "disconnected":
            if (status.reason === undefined) {
                return null;
            }
            return (
                <p className="status-message status-error">{status.reason}</p>
            );
        case "connected":
            return <p className="status-message">Loading world…</p>;
    }
}
