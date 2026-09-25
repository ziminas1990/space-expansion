import { Application, Container, Graphics } from "pixi.js";
import { useEffect, useRef } from "react";
import {
    predict_position,
    type Position,
    type Vector2D,
} from "../../common/domain/position.js";
import type { World } from "../../common/domain/world.js";
import {
    create_camera,
    follow,
    pan_by,
    release_follow,
    sync_follow,
    zoom_at,
    type Camera,
    type Point,
} from "./camera.js";
import { inscribed_square_extent } from "./inscribed_extent.js";
import { marker_display_scale } from "./marker_scale.js";
import { picture_rotation } from "./picture_rotation.js";
import { ASTEROID_PICTURE, PICTURE_SIZE, SHIP_PICTURE } from "./pictures.js";

const BACKGROUND = 0x050814;
const OUTDATED_ALPHA = 0.35;
const DETECTED_SHIP_SIZE = 20;
const ZOOM_STEP = 1.1;

type EntityKind = "asteroid" | "ship" | "player_ship";

type Marker = {
    kind: EntityKind;
    graphics: Graphics;
};

type PixiMapProps = {
    world: World;
    version: number;
    followed_ship_id: string | undefined;
    on_release_follow: () => void;
};

export function PixiMap({
    world,
    version,
    followed_ship_id,
    on_release_follow,
}: PixiMapProps) {
    const container_ref = useRef<HTMLDivElement>(null);
    const camera_ref = useRef<Camera>(create_camera());
    const world_ref = useRef(world);
    const version_ref = useRef(version);
    const on_release_ref = useRef(on_release_follow);

    world_ref.current = world;
    version_ref.current = version;
    on_release_ref.current = on_release_follow;

    useEffect(() => {
        if (followed_ship_id === undefined) {
            const camera = camera_ref.current;
            if (camera.followed_id !== undefined) {
                camera_ref.current = {
                    ...camera,
                    followed_id: undefined,
                };
            }
            return;
        }
        const ship = world.get_player_ship(followed_ship_id);
        if (ship === undefined) {
            return;
        }
        camera_ref.current = follow(
            camera_ref.current,
            followed_ship_id,
            predicted_xy(ship, world.now()),
        );
    }, [followed_ship_id, world]);

    useEffect(() => {
        const container = container_ref.current;
        if (container === null) {
            return;
        }

        const app = new Application();
        const world_layer = new Container();
        world_layer.sortableChildren = true;
        const markers = new Map<string, Marker>();
        let cancelled = false;
        let cleaned = false;
        let last_version = -1;

        let dragging = false;
        let drag_pointer_id: number | undefined;
        let last_pointer: Point | undefined;

        const stop_drag = (event?: PointerEvent) => {
            if (!dragging) {
                return;
            }
            if (
                event !== undefined
                && drag_pointer_id !== undefined
                && event.pointerId !== drag_pointer_id
            ) {
                return;
            }
            const canvas = app.canvas;
            if (
                canvas !== undefined
                && drag_pointer_id !== undefined
                && canvas.hasPointerCapture(drag_pointer_id)
            ) {
                canvas.releasePointerCapture(drag_pointer_id);
            }
            dragging = false;
            drag_pointer_id = undefined;
            last_pointer = undefined;
        };

        const on_pointer_down = (event: PointerEvent) => {
            if (event.button !== 0 || dragging) {
                return;
            }
            if (camera_ref.current.followed_id !== undefined) {
                return;
            }
            event.preventDefault();
            dragging = true;
            drag_pointer_id = event.pointerId;
            last_pointer = { x: event.clientX, y: event.clientY };
            app.canvas.setPointerCapture(event.pointerId);
        };

        const on_pointer_move = (event: PointerEvent) => {
            if (!dragging || last_pointer === undefined) {
                return;
            }
            if (event.pointerId !== drag_pointer_id) {
                return;
            }
            if (camera_ref.current.followed_id !== undefined) {
                stop_drag(event);
                return;
            }
            const delta = {
                x: event.clientX - last_pointer.x,
                y: event.clientY - last_pointer.y,
            };
            last_pointer = { x: event.clientX, y: event.clientY };
            camera_ref.current = pan_by(camera_ref.current, delta);
        };

        const on_pointer_up = (event: PointerEvent) => {
            stop_drag(event);
        };

        const on_wheel = (event: WheelEvent) => {
            event.preventDefault();
            const canvas = app.canvas;
            const bounds = canvas.getBoundingClientRect();
            if (bounds.width <= 0 || bounds.height <= 0) {
                return;
            }
            const pointer = {
                x: event.clientX - bounds.left,
                y: event.clientY - bounds.top,
            };
            const factor = event.deltaY < 0 ? ZOOM_STEP : 1 / ZOOM_STEP;
            camera_ref.current = zoom_at(
                camera_ref.current,
                { width: bounds.width, height: bounds.height },
                pointer,
                factor,
            );
        };

        const on_keydown = (event: KeyboardEvent) => {
            if (event.key !== "Escape") {
                return;
            }
            const camera = camera_ref.current;
            if (camera.followed_id === undefined) {
                return;
            }
            event.preventDefault();
            camera_ref.current = release_follow(camera);
            on_release_ref.current();
        };

        const on_tick = () => {
            const current_world = world_ref.current;
            let camera = camera_ref.current;
            if (camera.followed_id !== undefined) {
                const ship = current_world.get_player_ship(camera.followed_id);
                if (ship !== undefined) {
                    camera = sync_follow(camera, predicted_xy(ship, current_world.now()));
                    camera_ref.current = camera;
                }
            }

            if (last_version !== version_ref.current) {
                last_version = version_ref.current;
                reconcile(current_world, world_layer, markers);
            }

            update_markers(current_world, markers, current_world.now(), camera.scale);
            apply_camera(world_layer, camera, app.screen.width, app.screen.height);
            if (camera.followed_id !== undefined) {
                app.canvas.style.cursor = "default";
            } else {
                app.canvas.style.cursor = dragging ? "grabbing" : "grab";
            }
        };

        const cleanup_app = () => {
            if (cleaned) {
                return;
            }
            window.removeEventListener("keydown", on_keydown);
            if (app.renderer === undefined) {
                return;
            }
            cleaned = true;
            app.canvas.removeEventListener("pointerdown", on_pointer_down);
            app.canvas.removeEventListener("pointermove", on_pointer_move);
            app.canvas.removeEventListener("pointerup", on_pointer_up);
            app.canvas.removeEventListener("pointercancel", on_pointer_up);
            app.canvas.removeEventListener("wheel", on_wheel);
            app.ticker.remove(on_tick);
            app.destroy(true, { children: true });
        };

        void (async () => {
            try {
                await app.init({
                    resizeTo: container,
                    resolution: window.devicePixelRatio || 1,
                    autoDensity: true,
                    antialias: true,
                    background: BACKGROUND,
                });
            } catch {
                return;
            }
            if (cancelled) {
                cleanup_app();
                return;
            }
            app.stage.addChild(world_layer);
            container.appendChild(app.canvas);
            app.canvas.addEventListener("pointerdown", on_pointer_down);
            app.canvas.addEventListener("pointermove", on_pointer_move);
            app.canvas.addEventListener("pointerup", on_pointer_up);
            app.canvas.addEventListener("pointercancel", on_pointer_up);
            app.canvas.addEventListener("wheel", on_wheel, { passive: false });
            window.addEventListener("keydown", on_keydown);
            app.ticker.add(on_tick);
            if (cancelled) {
                cleanup_app();
            }
        })();

        return () => {
            cancelled = true;
            cleanup_app();
        };
    }, []);

    return <div className="map-stage" ref={container_ref} />;
}

function predicted_xy(
    entity: { get_position(): Position },
    now: number | undefined,
): Point {
    const position = entity.get_position();
    const predicted = predict_position(position, now ?? position.timestamp);
    return { x: predicted.x, y: predicted.y };
}

function entity_key(kind: EntityKind, id: string): string {
    return `${kind}:${id}`;
}

function apply_camera(
    layer: Container,
    camera: Camera,
    width: number,
    height: number,
): void {
    layer.scale.set(camera.scale);
    layer.position.set(
        width / 2 - camera.center.x * camera.scale,
        height / 2 - camera.center.y * camera.scale,
    );
}

function reconcile(
    world: World,
    layer: Container,
    markers: Map<string, Marker>,
): void {
    const seen = new Set<string>();

    for (const asteroid of world.get_asteroids()) {
        const key = entity_key("asteroid", asteroid.get_id());
        seen.add(key);
        if (markers.has(key)) {
            continue;
        }
        const graphics = create_picture(ASTEROID_PICTURE, 0);
        layer.addChild(graphics);
        markers.set(key, { kind: "asteroid", graphics });
    }

    for (const ship of world.get_detected_ships()) {
        const key = entity_key("ship", ship.get_id());
        seen.add(key);
        if (markers.has(key)) {
            continue;
        }
        const graphics = create_picture(SHIP_PICTURE, 1);
        layer.addChild(graphics);
        markers.set(key, { kind: "ship", graphics });
    }

    for (const ship of world.get_player_ships()) {
        const key = entity_key("player_ship", ship.get_id());
        seen.add(key);
        if (markers.has(key)) {
            continue;
        }
        const graphics = create_picture(SHIP_PICTURE, 2);
        layer.addChild(graphics);
        markers.set(key, { kind: "player_ship", graphics });
    }

    for (const [key, marker] of markers) {
        if (seen.has(key)) {
            continue;
        }
        marker.graphics.destroy();
        markers.delete(key);
    }
}

function update_markers(
    world: World,
    markers: Map<string, Marker>,
    now: number | undefined,
    camera_scale: number,
): void {
    for (const asteroid of world.get_asteroids()) {
        const marker = markers.get(entity_key("asteroid", asteroid.get_id()));
        if (marker === undefined) {
            continue;
        }
        place_picture(
            marker.graphics,
            predicted_xy(asteroid, now),
            asteroid_extent(asteroid.get_radius()),
            camera_scale,
            asteroid.get_orientation(),
            asteroid.outdated,
        );
    }

    for (const ship of world.get_detected_ships()) {
        const marker = markers.get(entity_key("ship", ship.get_id()));
        if (marker === undefined) {
            continue;
        }
        place_picture(
            marker.graphics,
            predicted_xy(ship, now),
            DETECTED_SHIP_SIZE,
            camera_scale,
            ship.get_orientation(),
            ship.outdated,
        );
    }

    for (const ship of world.get_player_ships()) {
        const marker = markers.get(entity_key("player_ship", ship.get_id()));
        if (marker === undefined) {
            continue;
        }
        place_picture(
            marker.graphics,
            predicted_xy(ship, now),
            inscribed_square_extent(ship.get_radius()),
            camera_scale,
            ship.get_orientation(),
            ship.outdated,
        );
    }
}

function asteroid_extent(radius: number): number {
    return Math.max(radius, 1) * 2;
}

function create_picture(svg: string, z_index: number): Graphics {
    const graphics = new Graphics();
    graphics.svg(svg);
    graphics.pivot.set(PICTURE_SIZE / 2, PICTURE_SIZE / 2);
    graphics.zIndex = z_index;
    return graphics;
}

function place_picture(
    graphics: Graphics,
    xy: Point,
    world_size: number,
    camera_scale: number,
    orientation: Vector2D | undefined,
    outdated: boolean,
): void {
    const boost = marker_display_scale(world_size, camera_scale);
    graphics.position.set(xy.x, xy.y);
    graphics.rotation = picture_rotation(orientation);
    graphics.alpha = outdated ? OUTDATED_ALPHA : 1;
    graphics.scale.set((world_size / PICTURE_SIZE) * boost);
}
