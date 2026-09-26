# TSSDK-005: Rework the tactical map ship list

## Background

The tactical map lists the player's ships in one flat column. A miner and a
scout sit side by side, and the list never says which type a ship is. The
column grows with every ship, and there is no way to fold one type away.

Choosing a ship selects it and follows it as one action. Escape drops both
at once. The chosen row is highlighted in blue.

The map also stops zooming out while the view still covers only a small
neighborhood. A region measured in billions of kilometers does not fit.

## Require

Rework that list, how a chosen ship is cleared, and how far the map can zoom
out.

The list is a column of blocks, one block per ship type. A ship's type is
the blueprint it was built from, such as Miner or Tiny-Scout. Ships of one
type share a block. The blocks follow type name order, and each block is
headed by that name.

- Every block starts expanded. The player can collapse or expand one block
  on its own.
- When the blocks do not fit, the list scrolls. The scrollbar belongs to
  the list.
- The top of the list has an "Only Visible" checkbox. It starts checked.
- While it is checked, a ship appears when it lies in the part of the
  world the map is showing. The selected ship remains in the list even
  when it is outside the view. A type with no visible or selected ship is
  left out of the list.
- While it is unchecked, every player ship is listed, still grouped by
  type.
- The set of ships treated as visible is refreshed about once a second,
  and not more often.

Choosing a ship still selects it and follows it. The selected ship's tile
is dark gold.

- Escape, while the map is following a ship, stops following. That ship
  stays selected, and the dark-gold tile stays.
- Escape, while the map is not following, clears the selection and the
  highlight.

The map can be zoomed out until the view is at least 10 billion kilometers
across.

## Acceptance

- Ships of one type sit in one block, headed by that type, and the blocks
  follow type name order.
- Every block starts open. Collapsing one hides its ships and leaves the
  other blocks open. Expanding it shows those ships again.
- A list that does not fit scrolls as a whole.
- "Only Visible" starts checked. A ship outside the current view is absent
  unless it is selected, and a type with no visible or selected ship is
  absent.
- Unchecking it lists those ships again, in their type blocks.
- Panning or zooming does not change which ships count as visible more than
  once a second.
- The selected ship's tile is dark gold.
- While the map is following a ship, Escape stops following and leaves that
  dark-gold tile.
- While the map is not following, Escape clears the selection and the
  highlight.
- The map can be zoomed out so the view covers at least 10 billion
  kilometers.
