import type { PlayerShip } from "../common/domain/player_ship.js";
import type { InstalledModule, ModuleInfo } from "../common/domain/module.js";
import { ResourceContainer } from "../common/domain/resource_container.js";
import { Shipyard } from "../common/domain/shipyard.js";

export function toggle_module_type(
    expanded_by_group: ReadonlyMap<string, ReadonlySet<string>>,
    group_name: string,
    type: string,
): ReadonlyMap<string, ReadonlySet<string>> {
    const next = new Map(expanded_by_group);
    const expanded = new Set(next.get(group_name) ?? []);
    if (expanded.has(type)) {
        expanded.delete(type);
    } else {
        expanded.add(type);
    }
    if (expanded.size === 0) {
        next.delete(group_name);
    } else {
        next.set(group_name, expanded);
    }
    return next;
}

export function toggle_module(
    expanded_by_group: ReadonlyMap<string, ReadonlySet<string>>,
    group_name: string,
    module_key: string,
): ReadonlyMap<string, ReadonlySet<string>> {
    const next = new Map(expanded_by_group);
    const expanded = new Set(next.get(group_name) ?? []);
    if (expanded.has(module_key)) {
        expanded.delete(module_key);
    } else {
        expanded.add(module_key);
    }
    if (expanded.size === 0) {
        next.delete(group_name);
    } else {
        next.set(group_name, expanded);
    }
    return next;
}

export function module_expansion_key(module: ModuleInfo): string {
    return JSON.stringify([module.type, module.name]);
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
    expanded_modules: ReadonlySet<string>;
    on_toggle_type: (type: string) => void;
    on_toggle_module: (module_key: string) => void;
};

export function ShipPanel({
    ship,
    expanded_types,
    expanded_modules,
    on_toggle_type,
    on_toggle_module,
}: ShipPanelProps) {
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
                                        {module instanceof ResourceContainer ? (
                                            <ResourceContainerWidget
                                                module={module}
                                                expanded={expanded_modules.has(module_expansion_key(module))}
                                                on_toggle={() => on_toggle_module(module_expansion_key(module))}
                                            />
                                        ) : module instanceof Shipyard ? (
                                            <ShipyardWidget
                                                module={module}
                                                expanded={expanded_modules.has(module_expansion_key(module))}
                                                on_toggle={() => on_toggle_module(module_expansion_key(module))}
                                            />
                                        ) : (
                                            <h4>{module.name}</h4>
                                        )}
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

function ShipyardWidget({
    module,
    expanded,
    on_toggle,
}: {
    module: Shipyard;
    expanded: boolean;
    on_toggle: () => void;
}) {
    const state = module.get_state();
    const percent = state !== undefined && state.status !== "idle"
        ? Math.round(state.progress * 100)
        : undefined;
    const status = state === undefined
        ? "…"
        : state.status === "idle"
            ? "idle"
            : `${state.status} ${percent}%`;

    return (
        <details className="shipyard-widget" open={expanded}>
            <summary onClick={(event) => {
                event.preventDefault();
                on_toggle();
            }}>
                <span>{module.name}</span>
                <span className="shipyard-status">{status}</span>
            </summary>
            {state !== undefined && state.status !== "idle" && percent !== undefined && (
                <div className="shipyard-build">
                    <p>Type: {state.blueprint_name}</p>
                    <p>Ordered name: {state.ship_name}</p>
                    <label>
                        Progress: {percent}%
                        <progress max={100} value={percent} />
                    </label>
                </div>
            )}
        </details>
    );
}

function ResourceContainerWidget({
    module,
    expanded,
    on_toggle,
}: {
    module: ResourceContainer;
    expanded: boolean;
    on_toggle: () => void;
}) {
    const content = module.get_content();
    const percent = content === undefined
        ? "—"
        : `${content.volume > 0
            ? Math.round(100 * content.used / content.volume)
            : 0}%`;
    const resources = content?.resources
        .filter((resource) => resource.amount > 0)
        .sort((a, b) => b.amount - a.amount || a.resource_type.localeCompare(b.resource_type))
        ?? [];

    return (
        <details className="resource-container-widget" open={expanded}>
            <summary onClick={(event) => {
                event.preventDefault();
                on_toggle();
            }}>
                <span>{module.name}</span>
                <span className="resource-container-fill">{percent}</span>
            </summary>
            {resources.length > 0 ? (
                <ul className="resource-container-resources">
                    {resources.map((resource) => (
                        <li key={resource.resource_type}>
                            <span>{resource.resource_type}</span>
                            <span>{Math.round(resource.amount).toLocaleString("en-US")} kg</span>
                        </li>
                    ))}
                </ul>
            ) : (
                <p className="resource-container-empty">
                    {content === undefined ? "Loading contents…" : "Empty"}
                </p>
            )}
        </details>
    );
}
