import { expect, test } from "vitest";
import { picture_rotation } from "./picture_rotation.js";

test("turns a picture from nose-up with the orientation", () => {
    // 1. unknown orientation and (0, 1) leave the picture as drawn, nose up
    expect(picture_rotation(undefined)).toBe(0);
    expect(picture_rotation({ x: 0, y: 1 })).toBe(0);
    expect(picture_rotation({ x: 0, y: 2 })).toBeCloseTo(0);
    expect(picture_rotation({ x: 0, y: 0 })).toBe(0);

    // 2. (1, 0) points the nose along +X
    expect(picture_rotation({ x: 1, y: 0 })).toBeCloseTo(Math.PI / 2);

    // 3. (-1, 0) points the nose along -X
    expect(picture_rotation({ x: -1, y: 0 })).toBeCloseTo(-Math.PI / 2);
});
