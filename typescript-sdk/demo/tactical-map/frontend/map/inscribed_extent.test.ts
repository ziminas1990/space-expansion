import { expect, test } from "vitest";
import { inscribed_square_extent } from "./inscribed_extent.js";
import { MIN_SCREEN_SIZE, marker_display_scale } from "./marker_scale.js";

function display_side(radius: number, camera_scale: number): number {
    const extent = inscribed_square_extent(radius);
    return extent * camera_scale * marker_display_scale(extent, camera_scale);
}

test("inscribes a ship picture in the hull circle and keeps it visible", () => {
    // 1. the square's corners meet a circle of the hull radius
    const radius = 20;
    const side = inscribed_square_extent(radius);
    expect((side / 2) * Math.SQRT2).toBeCloseTo(radius);

    // 2. twice the hull radius is twice as large when both pictures clear 10 pixels
    const close = 1;
    expect(display_side(radius, close)).toBeGreaterThanOrEqual(MIN_SCREEN_SIZE);
    expect(display_side(radius * 2, close)).toBeGreaterThanOrEqual(MIN_SCREEN_SIZE);
    expect(display_side(radius * 2, close)).toBeCloseTo(display_side(radius, close) * 2);

    // 3. zoomed further out, the picture stays at least 10 by 10 pixels
    expect(display_side(radius, 0.01)).toBeCloseTo(MIN_SCREEN_SIZE);
});
