import type { World } from "../common/domain/world.js";

type ShipListProps = {
    world: World;
    followed_ship_id: string | undefined;
    on_select: (id: string) => void;
};

export function ShipList({ world, followed_ship_id, on_select }: ShipListProps) {
    const ships = world.get_player_ships();

    return (
        <aside className="ship-list">
            <h2>Player ships</h2>
            {ships.length === 0 ? (
                <p className="ship-list-empty">No player ships</p>
            ) : (
                <ul>
                    {ships.map((ship) => {
                        const id = ship.get_id();
                        const selected = id === followed_ship_id;
                        return (
                            <li key={id}>
                                <button
                                    type="button"
                                    className={selected ? "ship-entry selected" : "ship-entry"}
                                    aria-pressed={selected}
                                    onClick={() => on_select(id)}
                                >
                                    <span className="ship-name">{id}</span>
                                    {selected && (
                                        <span className="ship-follow-state">Following</span>
                                    )}
                                </button>
                            </li>
                        );
                    })}
                </ul>
            )}
        </aside>
    );
}
