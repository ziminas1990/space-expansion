import { expect, test } from "vitest";
import {
    MAX_SCALE,
    MIN_SCALE,
    create_camera,
    follow,
    release_follow,
    screen_to_world,
    sync_follow,
    world_to_screen,
    zoom_at,
} from "./camera.js";

const VIEWPORT = { width: 800, height: 600 };

test("converts between screen and world coordinates", () => {
    // 1. place the camera on a world point with a non-unit scale
    const camera = create_camera({
        center: { x: 100, y: 200 },
        scale: 2,
    });

    // 2. the camera center must map to the viewport midpoint
    expect(world_to_screen({ x: 100, y: 200 }, camera, VIEWPORT)).toEqual({
        x: 400,
        y: 300,
    });

    // 3. an offset world point maps by scale, and converts back
    const screen = world_to_screen({ x: 150, y: 180 }, camera, VIEWPORT);
    expect(screen).toEqual({ x: 500, y: 260 });
    expect(screen_to_world(screen, camera, VIEWPORT)).toEqual({
        x: 150,
        y: 180,
    });
});

test("zooms around the pointer without moving the world point under it", () => {
    // 1. record the world point under an off-center pointer
    const camera = create_camera({
        center: { x: 40, y: -10 },
        scale: 1,
    });
    const pointer = { x: 620, y: 140 };
    const world_before = screen_to_world(pointer, camera, VIEWPORT);

    // 2. zoom in around that pointer
    const zoomed = zoom_at(camera, VIEWPORT, pointer, 2);
    expect(zoomed.scale).toBe(2);
    const world_after = screen_to_world(pointer, zoomed, VIEWPORT);
    expect(world_after.x).toBeCloseTo(world_before.x);
    expect(world_after.y).toBeCloseTo(world_before.y);

    // 3. zoom out around the same pointer
    const zoomed_out = zoom_at(zoomed, VIEWPORT, pointer, 0.5);
    expect(zoomed_out.scale).toBeCloseTo(1);
    const world_out = screen_to_world(pointer, zoomed_out, VIEWPORT);
    expect(world_out.x).toBeCloseTo(world_before.x);
    expect(world_out.y).toBeCloseTo(world_before.y);

    // 4. clamping at the max scale still keeps the point under the pointer
    const clamped = zoom_at(zoomed, VIEWPORT, pointer, 1e9);
    expect(clamped.scale).toBe(MAX_SCALE);
    const world_clamped = screen_to_world(pointer, clamped, VIEWPORT);
    expect(world_clamped.x).toBeCloseTo(world_before.x);
    expect(world_clamped.y).toBeCloseTo(world_before.y);

    // 5. clamping at the min scale does the same
    const minned = zoom_at(camera, VIEWPORT, pointer, 1e-9);
    expect(minned.scale).toBe(MIN_SCALE);
    const world_minned = screen_to_world(pointer, minned, VIEWPORT);
    expect(world_minned.x).toBeCloseTo(world_before.x);
    expect(world_minned.y).toBeCloseTo(world_before.y);
});

test("escape stops follow without moving the camera", () => {
    // 1. start following a ship and snap to its position
    let camera = follow(create_camera(), "Scout", { x: 10, y: 20 });
    expect(camera.followed_id).toBe("Scout");
    expect(camera.center).toEqual({ x: 10, y: 20 });

    // 2. the followed ship moves and the camera tracks it
    camera = sync_follow(camera, { x: 30, y: 40 });
    expect(camera.center).toEqual({ x: 30, y: 40 });

    // 3. escape preserves the current center and clears follow
    camera = release_follow(camera);
    expect(camera.followed_id).toBeUndefined();
    expect(camera.center).toEqual({ x: 30, y: 40 });

    // 4. later ship motion does not move the camera
    camera = sync_follow(camera, { x: 99, y: 120 });
    expect(camera.center).toEqual({ x: 30, y: 40 });
});
