import { useState } from "react";
import type { PlayerShip } from "../common/domain/player_ship.js";
import type { World } from "../common/domain/world.js";

type ShipListProps = {
    world: World;
    visible_ship_ids: ReadonlySet<string>;
    selected_ship_id: string | undefined;
    followed_ship_id: string | undefined;
    on_select: (id: string) => void;
};

export function group_player_ships(ships: readonly PlayerShip[]): [string, PlayerShip[]][] {
    const groups = new Map<string, PlayerShip[]>();
    for (const ship of ships) {
        const type = ship.get_blueprint_name();
        const group = groups.get(type) ?? [];
        group.push(ship);
        groups.set(type, group);
    }
    return [...groups].sort(([a], [b]) => a.localeCompare(b));
}

export function ShipList({
    world,
    visible_ship_ids,
    selected_ship_id,
    followed_ship_id,
    on_select,
}: ShipListProps) {
    const [only_visible, set_only_visible] = useState(true);
    const [collapsed_types, set_collapsed_types] = useState<ReadonlySet<string>>(
        () => new Set(),
    );
    const ships = world.get_player_ships().filter(
        (ship) => !only_visible
            || visible_ship_ids.has(ship.get_id())
            || ship.get_id() === selected_ship_id,
    );
    const groups = group_player_ships(ships);

    function toggle_type(type: string): void {
        set_collapsed_types((previous) => {
            const next = new Set(previous);
            if (next.has(type)) {
                next.delete(type);
            } else {
                next.add(type);
            }
            return next;
        });
    }

    return (
        <aside className="ship-list">
            <h2>Player ships</h2>
            <label className="ship-list-filter">
                <input
                    type="checkbox"
                    checked={only_visible}
                    onChange={(event) => set_only_visible(event.target.checked)}
                />
                Only Visible
            </label>
            {groups.length === 0 ? (
                <p className="ship-list-empty">
                    {only_visible ? "No player ships in view" : "No player ships"}
                </p>
            ) : groups.map(([type, group]) => {
                const expanded = !collapsed_types.has(type);
                return (
                    <section className="ship-group" key={type}>
                        <h3>
                            <button
                                type="button"
                                className="ship-group-toggle"
                                aria-expanded={expanded}
                                onClick={() => toggle_type(type)}
                            >
                                <span aria-hidden="true">{expanded ? "▾" : "▸"}</span>
                                <span>{type}</span>
                                <span className="ship-group-count">{group.length}</span>
                            </button>
                        </h3>
                        {expanded && (
                            <ul>
                                {group.map((ship) => {
                                    const id = ship.get_id();
                                    const selected = id === selected_ship_id;
                                    return (
                                        <li key={id}>
                                            <button
                                                type="button"
                                                className={selected ? "ship-entry selected" : "ship-entry"}
                                                aria-pressed={selected}
                                                onClick={() => on_select(id)}
                                            >
                                                <span className="ship-name">{id}</span>
                                                {selected && id === followed_ship_id && (
                                                    <span className="ship-follow-state">Following</span>
                                                )}
                                            </button>
                                        </li>
                                    );
                                })}
                            </ul>
                        )}
                    </section>
                );
            })}
        </aside>
    );
}
