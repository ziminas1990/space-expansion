import { useState, useSyncExternalStore } from "react";
import { LoginForm } from "./LoginForm.js";
import { PixiMap } from "./map/PixiMap.js";
import { ShipList } from "./ShipList.js";
import { ShipPanel, toggle_module, toggle_module_type } from "./ShipPanel.js";
import {
    WorldClient,
    type ClientStatus,
    type LoginCredentials,
} from "./world_client.js";

export function App() {
    const [client] = useState(() => new WorldClient());
    const [visible_ship_ids, set_visible_ship_ids] = useState<ReadonlySet<string>>(
        () => new Set(),
    );
    const [expanded_module_types, set_expanded_module_types] = useState<
        ReadonlyMap<string, ReadonlySet<string>>
    >(() => new Map());
    const [expanded_modules, set_expanded_modules] = useState<
        ReadonlyMap<string, ReadonlySet<string>>
    >(() => new Map());
    const version = useSyncExternalStore(client.subscribe, client.get_version);
    const status = client.get_status();
    const world = client.get_world();
    const followed_ship_id = client.get_followed_ship_id();
    const selected_ship_id = client.get_selected_ship_id();

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

    const focused_ship = selected_ship_id === undefined
        ? undefined
        : world.get_player_ship(selected_ship_id);
    const focused_group_name = focused_ship?.get_blueprint_name();

    return (
        <div className="app tactical-screen" data-version={version}>
            <PixiMap
                world={world}
                version={version}
                followed_ship_id={followed_ship_id}
                selected_ship_id={selected_ship_id}
                on_stop_following={() => client.stop_following()}
                on_clear_selection={() => client.clear_selection()}
                on_visible_ships_change={set_visible_ship_ids}
            />
            <div className="connection-chip">Connected</div>
            <ShipList
                world={world}
                visible_ship_ids={visible_ship_ids}
                selected_ship_id={selected_ship_id}
                followed_ship_id={followed_ship_id}
                on_select={(id) => client.select_ship(id)}
            />
            {focused_ship !== undefined && focused_group_name !== undefined && (
                <ShipPanel
                    key={focused_ship.get_id()}
                    ship={focused_ship}
                    expanded_types={expanded_module_types.get(focused_group_name) ?? new Set()}
                    expanded_modules={expanded_modules.get(focused_group_name) ?? new Set()}
                    on_toggle_type={(type) => set_expanded_module_types((previous) =>
                        toggle_module_type(previous, focused_group_name, type)
                    )}
                    on_toggle_module={(module_key) => set_expanded_modules((previous) =>
                        toggle_module(previous, focused_group_name, module_key)
                    )}
                />
            )}
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
