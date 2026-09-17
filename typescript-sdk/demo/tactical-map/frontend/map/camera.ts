export const MIN_SCALE = 0.01;
export const MAX_SCALE = 64;
export const DEFAULT_SCALE = 1;

export type Point = {
    x: number;
    y: number;
};

export type Viewport = {
    width: number;
    height: number;
};

export type Camera = {
    center: Point;
    scale: number;
    followed_id: string | undefined;
};

export type CameraOptions = {
    center?: Point;
    scale?: number;
    followed_id?: string;
};

export function clamp_scale(scale: number): number {
    if (scale < MIN_SCALE) {
        return MIN_SCALE;
    }
    if (scale > MAX_SCALE) {
        return MAX_SCALE;
    }
    return scale;
}

export function create_camera(options: CameraOptions = {}): Camera {
    const center = options.center ?? { x: 0, y: 0 };
    return {
        center: { x: center.x, y: center.y },
        scale: clamp_scale(options.scale ?? DEFAULT_SCALE),
        followed_id: options.followed_id,
    };
}

export function world_to_screen(
    world: Point,
    camera: Camera,
    viewport: Viewport,
): Point {
    return {
        x: (world.x - camera.center.x) * camera.scale + viewport.width / 2,
        y: (world.y - camera.center.y) * camera.scale + viewport.height / 2,
    };
}

export function screen_to_world(
    screen: Point,
    camera: Camera,
    viewport: Viewport,
): Point {
    return {
        x: (screen.x - viewport.width / 2) / camera.scale + camera.center.x,
        y: (screen.y - viewport.height / 2) / camera.scale + camera.center.y,
    };
}

export function zoom_at(
    camera: Camera,
    viewport: Viewport,
    pointer: Point,
    scale_factor: number,
): Camera {
    const world = screen_to_world(pointer, camera, viewport);
    const scale = clamp_scale(camera.scale * scale_factor);
    return {
        ...camera,
        scale,
        center: {
            x: world.x - (pointer.x - viewport.width / 2) / scale,
            y: world.y - (pointer.y - viewport.height / 2) / scale,
        },
    };
}

export function follow(camera: Camera, id: string, position: Point): Camera {
    return {
        ...camera,
        followed_id: id,
        center: { x: position.x, y: position.y },
    };
}

export function sync_follow(camera: Camera, position: Point): Camera {
    if (camera.followed_id === undefined) {
        return camera;
    }
    return {
        ...camera,
        center: { x: position.x, y: position.y },
    };
}

export function release_follow(camera: Camera): Camera {
    return {
        ...camera,
        followed_id: undefined,
        center: { x: camera.center.x, y: camera.center.y },
    };
}
