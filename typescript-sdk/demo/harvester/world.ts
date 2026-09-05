import type { PhysicalObject } from "../../types/index.js";

export class World {
    readonly asteroids = new Map<number, PhysicalObject>();

    update_object(object: PhysicalObject): void {
        if (object.object_type !== "asteroid") {
            return;
        }
        const cached = this.asteroids.get(object.object_id);
        if (!cached || object.position.timestamp >= cached.position.timestamp) {
            this.asteroids.set(object.object_id, {
                object_type: object.object_type,
                object_id: object.object_id,
                position: object.position,
                radius: object.radius,
            });
        }
    }

    update_objects(objects: Iterable<PhysicalObject>): void {
        for (const object of objects) {
            this.update_object(object);
        }
    }
}
