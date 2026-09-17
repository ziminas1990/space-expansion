import { Application, Container, Graphics } from "pixi.js";
import { useEffect, useRef } from "react";
import { predict_position, type Position } from "../../common/domain/position.js";
import type { World } from "../../common/domain/world.js";
import {
    create_camera,
    follow,
    release_follow,
    sync_follow,
    zoom_at,
    type Camera,
    type Point,
} from "./camera.js";
import { marker_display_scale } from "./marker_scale.js";

const BACKGROUND = 0x050814;
const OUTDATED_ALPHA = 0.35;
const PLAYER_SHIP_SIZE = 12;
const DETECTED_SHIP_SIZE = 10;
const ASTEROID_COLOR = 0x8b93a7;
const PLAYER_SHIP_COLOR = 0x4da3ff;
const DETECTED_SHIP_COLOR = 0xe08a3c;
const ZOOM_STEP = 1.1;

type EntityKind = "asteroid" | "ship" | "player_ship";

type Marker = {
    kind: EntityKind;
    graphics: Graphics;
    last_radius?: number;
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
        const radius = Math.max(asteroid.get_radius(), 1);
        const existing = markers.get(key);
        if (existing === undefined) {
            const graphics = new Graphics();
            graphics.zIndex = 0;
            draw_asteroid(graphics, radius);
            layer.addChild(graphics);
            markers.set(key, { kind: "asteroid", graphics, last_radius: radius });
        } else if (existing.last_radius !== radius) {
            draw_asteroid(existing.graphics, radius);
            existing.last_radius = radius;
        }
    }

    for (const ship of world.get_detected_ships()) {
        const key = entity_key("ship", ship.get_id());
        seen.add(key);
        if (markers.has(key)) {
            continue;
        }
        const graphics = new Graphics();
        graphics.zIndex = 1;
        draw_detected_ship(graphics);
        layer.addChild(graphics);
        markers.set(key, { kind: "ship", graphics });
    }

    for (const ship of world.get_player_ships()) {
        const key = entity_key("player_ship", ship.get_id());
        seen.add(key);
        if (markers.has(key)) {
            continue;
        }
        const graphics = new Graphics();
        graphics.zIndex = 2;
        draw_player_ship(graphics);
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
        const xy = predicted_xy(asteroid, now);
        marker.graphics.position.set(xy.x, xy.y);
        marker.graphics.alpha = asteroid.outdated ? OUTDATED_ALPHA : 1;
        marker.graphics.scale.set(
            marker_display_scale(marker.last_radius ?? 1, camera_scale),
        );
    }

    for (const ship of world.get_detected_ships()) {
        const marker = markers.get(entity_key("ship", ship.get_id()));
        if (marker === undefined) {
            continue;
        }
        const xy = predicted_xy(ship, now);
        marker.graphics.position.set(xy.x, xy.y);
        marker.graphics.alpha = ship.outdated ? OUTDATED_ALPHA : 1;
        marker.graphics.scale.set(marker_display_scale(DETECTED_SHIP_SIZE, camera_scale));
    }

    for (const ship of world.get_player_ships()) {
        const marker = markers.get(entity_key("player_ship", ship.get_id()));
        if (marker === undefined) {
            continue;
        }
        const xy = predicted_xy(ship, now);
        marker.graphics.position.set(xy.x, xy.y);
        marker.graphics.alpha = ship.outdated ? OUTDATED_ALPHA : 1;
        marker.graphics.scale.set(marker_display_scale(PLAYER_SHIP_SIZE, camera_scale));
    }
}

function draw_asteroid(graphics: Graphics, radius: number): void {
    graphics.clear();
    graphics.circle(0, 0, radius);
    graphics.fill(ASTEROID_COLOR);
}

function draw_player_ship(graphics: Graphics): void {
    const size = PLAYER_SHIP_SIZE;
    graphics.clear();
    graphics.poly([
        0, -size,
        size * 0.7, size * 0.6,
        -size * 0.7, size * 0.6,
    ]);
    graphics.fill(PLAYER_SHIP_COLOR);
}

function draw_detected_ship(graphics: Graphics): void {
    const size = DETECTED_SHIP_SIZE;
    graphics.clear();
    graphics.poly([
        0, -size,
        size, 0,
        0, size,
        -size, 0,
    ]);
    graphics.fill(DETECTED_SHIP_COLOR);
}
