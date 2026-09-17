export const MIN_SCREEN_RADIUS = 3;

export function marker_display_scale(
    world_radius: number,
    camera_scale: number,
    min_screen_radius: number = MIN_SCREEN_RADIUS,
): number {
    const screen_radius = world_radius * camera_scale;
    if (screen_radius >= min_screen_radius) {
        return 1;
    }
    if (world_radius <= 0 || camera_scale <= 0) {
        return 1;
    }
    return min_screen_radius / screen_radius;
}
