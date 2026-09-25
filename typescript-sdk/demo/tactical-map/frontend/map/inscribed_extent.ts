// Side of a square picture inscribed in a circle of `radius`.
// The corners meet the circle. The caller places the center on the entity.
export function inscribed_square_extent(radius: number): number {
    return radius * Math.SQRT2;
}
