# TSSDK-002: Draw ship pictures to the hull's facing and size

## Background

The tactical map draws every ship the player controls as one small picture
of a fixed size. A large hull and a small hull occupy the same spot.

The ship's state reports the nose direction together with the position.
Monitoring that state keeps the nose on the client beside the position, and
the map can turn the picture as the nose changes. The ship's specification
reports the hull radius, in meters. That radius is the circle the picture
belongs in. The center of the circle is the ship's position.

## Require

On the tactical map, draw each ship the player controls so the picture
matches that ship in the game world.

- Turn the picture with the nose from the ship's monitored state. The
  picture's default pose, nose up, is the direction (0, 1). The direction
  (1, 0) points the nose along positive X.
- When the nose is unknown, leave the picture in that default pose.
- When a later state reports a new nose, turn the picture with it.
- Inscribe the picture in a circle whose radius is the hull radius from
  the ship's specification. The whole picture stays inside that circle and
  meets it. The center of the circle is the ship's position.
- Ships with different hull radii are drawn in proportion to those radii.

## Acceptance

- A ship whose nose is (0, 1) shows the picture as drawn, nose up. One
  whose nose is (1, 0) shows the nose along positive X.
- While the ship turns, the picture turns with the nose from the monitored
  state.
- A ship whose nose is unknown shows the same picture as nose (0, 1).
- The picture lies inside a circle of that ship's hull radius, centered on
  the ship's position, and the picture meets the circle.
- A ship with twice the hull radius is drawn twice as large, when both
  circles are at least 10 pixels across on screen.
- Zoomed further out, the picture stays at least 10 by 10 pixels.
