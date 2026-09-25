import { expect, test } from "vitest";
import { picture_rotation } from "./picture_rotation.js";

test("turns a picture from nose-up onto the orientation", () => {
    // 1. unknown, zero, and (0, -1) leave the picture nose-up
    expect(picture_rotation(undefined)).toBe(0);
    expect(picture_rotation({ x: 0, y: 0 })).toBe(0);
    expect(picture_rotation({ x: 0, y: -1 })).toBe(0);
    expect(picture_rotation({ x: 0, y: -2 })).toBeCloseTo(0);

    // 2. (0, 1) points the nose down the screen, along world +Y
    expect(picture_rotation({ x: 0, y: 1 })).toBeCloseTo(Math.PI);
    expect(picture_rotation({ x: 0, y: 2 })).toBeCloseTo(Math.PI);

    // 3. (1, 0) points the nose along +X
    expect(picture_rotation({ x: 1, y: 0 })).toBeCloseTo(Math.PI / 2);

    // 4. (-1, 0) points the nose along -X
    expect(picture_rotation({ x: -1, y: 0 })).toBeCloseTo(-Math.PI / 2);

    // 5. (1, 1) points the nose down and right
    expect(picture_rotation({ x: 1, y: 1 })).toBeCloseTo((3 * Math.PI) / 4);
});
