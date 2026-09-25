# TSSDK-001: Show ships and asteroids as pictures on the tactical map

## Background

The tactical map draws an asteroid as a circle and a ship as a small
polygon. A player does not see a picture of either.

Pulling the view back shrinks those marks until they are only a few
pixels across. A ship and an asteroid are then hard to see and hard to
tell apart.

Every picture has the default orientation (0, 1): the nose points up,
along positive Y. The map does not turn the picture when a ship or an
asteroid faces another way. When the facing is unknown, the picture
stays in that default.

## Require

Draw asteroids and ships from pictures.

- An asteroid uses an asteroid picture. A ship uses a ship picture,
  whether it is the player's ship or a detected ship.
- At any map zoom, a picture stays at least 10 by 10 pixels on screen.
  It may be larger.
- Turn the picture with the entity's orientation. Every picture has the
  default orientation (0, 1), nose up. Orientation (1, 0) points the nose
  along positive X. The same rule applies to a ship and to an asteroid.
- When an object's orientation is unknown, draw its picture as
  orientation (0, 1).

## Acceptance

- On the map, an asteroid is the asteroid picture and a ship is the ship
  picture.
- Zoomed far out, each of those pictures is still at least 10 by 10
  pixels.
- A ship or an asteroid with orientation (0, 1) shows the picture as
  drawn, nose up. One with orientation (1, 0) shows the nose along the
  X axis. One whose orientation is unknown shows the same picture as
  orientation (0, 1).
