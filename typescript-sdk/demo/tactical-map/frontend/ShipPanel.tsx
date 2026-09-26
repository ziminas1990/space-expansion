import type { InstalledModule, PlayerShip } from "../common/domain/player_ship.js";

export function toggle_module_type(
    expanded_by_ship: ReadonlyMap<string, ReadonlySet<string>>,
    ship_id: string,
    type: string,
): ReadonlyMap<string, ReadonlySet<string>> {
    const next = new Map(expanded_by_ship);
    const expanded = new Set(next.get(ship_id) ?? []);
    if (expanded.has(type)) {
        expanded.delete(type);
    } else {
        expanded.add(type);
    }
    if (expanded.size === 0) {
        next.delete(ship_id);
    } else {
        next.set(ship_id, expanded);
    }
    return next;
}

export function group_modules(modules: readonly InstalledModule[]): [string, InstalledModule[]][] {
    const groups = new Map<string, InstalledModule[]>();
    for (const module of modules) {
        const group = groups.get(module.type) ?? [];
        group.push(module);
        groups.set(module.type, group);
    }
    return [...groups].sort(([a], [b]) => a.localeCompare(b));
}

type ShipPanelProps = {
    ship: PlayerShip;
    expanded_types: ReadonlySet<string>;
    on_toggle_type: (type: string) => void;
};

export function ShipPanel({ ship, expanded_types, on_toggle_type }: ShipPanelProps) {
    const groups = group_modules(ship.get_modules());

    return (
        <aside className="ship-panel" aria-label="Focused ship">
            <h2>{ship.get_id()}</h2>
            <p className="ship-panel-type">{ship.get_blueprint_name()}</p>
            {groups.map(([type, modules]) => {
                const expanded = expanded_types.has(type);
                return (
                    <section className="module-group" key={type}>
                        <h3>
                            <button
                                type="button"
                                className="module-group-toggle"
                                aria-expanded={expanded}
                                onClick={() => on_toggle_type(type)}
                            >
                                <span aria-hidden="true">{expanded ? "▾" : "▸"}</span>
                                <span>{type}</span>
                            </button>
                        </h3>
                        {expanded && (
                            <ul>
                                {modules.map((module) => (
                                    <li className="module-entry" key={module.slot_id}>
                                        <h4>{module.name}</h4>
                                    </li>
                                ))}
                            </ul>
                        )}
                    </section>
                );
            })}
        </aside>
    );
}
