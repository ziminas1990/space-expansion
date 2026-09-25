// Side of a picture on screen, in pixels. Zooming out must not shrink past this.
export const MIN_SCREEN_SIZE = 10;

export function marker_display_scale(
    world_size: number,
    camera_scale: number,
    min_screen_size: number = MIN_SCREEN_SIZE,
): number {
    const screen_size = world_size * camera_scale;
    if (screen_size >= min_screen_size) {
        return 1;
    }
    if (world_size <= 0 || camera_scale <= 0) {
        return 1;
    }
    return min_screen_size / screen_size;
}
