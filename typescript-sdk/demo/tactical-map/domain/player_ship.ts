import { unpack_position } from "./position.js";
import { Ship, ShipPacked } from "./ship.js";

export class PlayerShip extends Ship {

    static override unpack(packed: ShipPacked): PlayerShip {
        const [id, position, outdated] = packed;
        const ship = new PlayerShip(id, unpack_position(position));
        ship.outdated = outdated;
        return ship;
    }

}
