import { expect, test } from "vitest";
import { MIN_SCALE } from "./camera.js";
import { MIN_SCREEN_RADIUS, marker_display_scale } from "./marker_scale.js";

test("keeps markers at least a few pixels when zoomed far out", () => {
    // 1. a large object already fills more than the minimum, so it is not boosted
    expect(marker_display_scale(50, 1)).toBe(1);

    // 2. a small ship at the minimum camera scale would vanish without a boost
    const boosted = marker_display_scale(12, MIN_SCALE);
    expect(12 * MIN_SCALE * boosted).toBeCloseTo(MIN_SCREEN_RADIUS);

    // 3. zoomed in, the same ship keeps its true world size
    expect(marker_display_scale(12, 8)).toBe(1);
});
