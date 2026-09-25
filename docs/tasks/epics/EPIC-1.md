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

The TypeScript harvester then flies on that main engine. Its navigation
is rewritten for a turn and a burn, and in that same step the RCS maximum
thrust is lowered. The thrust those flights use moves to the hover engine.
The hull thrusters stay able to push in any direction, at less force than
they have now.

After that, IRCS no longer takes a magnitude. The thrusters push at their
full force, and the player sets only the direction. The protocol replaces
the command that carried a magnitude: the fields of the existing IRCS
commands may be renumbered, and clients move with the server.

## Steps

The steps are done in this order. The ship must be able to face a
direction before the main engine can be aimed, and the harvester switches
only after that engine exists.

1. [SES-227](../closed/SES-227.md) — give the ship an orientation, a
   maximum turn rate, and a way to rotate.
2. [SES-228](../closed/SES-228.md) — rename IEngine to IRCS without
   changing how clients fly.
3. [SES-229](../closed/SES-229.md) — add IHoverEngine, the main engine
   that thrusts along the nose.
4. [SES-230](../closed/SES-230.md) — rewrite harvester navigation for
   IHoverEngine and lower the RCS maximum thrust.
5. [SES-232](../SES-232.md) — IRCS thrusts at its maximum; the player
   sets only the direction.
