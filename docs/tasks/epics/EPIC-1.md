# EPIC-1: Turn the ship, then fire the main engine

A ship today is a point with a position and a velocity. Its engine accepts
a thrust vector in any direction and can change that vector at once.
Maneuvering is the choice of a force. Facing the ship never comes into it.

This epic makes maneuvering a turn and a burn. The ship gains a nose and a
limit on how fast it can turn. The player aims by rotating the ship, then
fires the main engine along that facing.

Propulsion splits in two. The interface that is IEngine today becomes
IRCS: thrusters built into the hull, able to push in any direction. They
keep their present strength, so clients that already fly by choosing a
thrust vector keep that logic and only take the new name. IHoverEngine is
the new main engine. It thrusts only along the ship's nose.

The TypeScript harvester then flies on that main engine. These steps do
not lower the RCS maximum thrust. That reduction comes later, once the
clients that should use the main engine have moved.

## Steps

The steps are done in this order. The ship must be able to face a
direction before the main engine can be aimed, and the harvester switches
only after that engine exists.

1. [SES-227](../SES-227.md) — give the ship an orientation, a
   maximum turn rate, and a way to rotate.
2. [SES-228](../SES-228.md) — rename IEngine to IRCS without
   changing how clients fly.
3. [SES-229](../SES-229.md) — add IHoverEngine, the main engine
   that thrusts along the nose.
4. [SES-230](../SES-230.md) — fly the TypeScript harvester on IHoverEngine.
