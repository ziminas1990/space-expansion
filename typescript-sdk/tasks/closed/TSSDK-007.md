# TSSDK-007: Show the focused ship's modules in a side panel

## Background

The tactical map lets the player select a ship, but the selection only
highlights it in the ship list and on the map. There is no place to see which
ship was selected or what modules are installed on it.

## Require

Show a ship panel on the right side of the tactical map when the player selects
a ship and the map focuses on it. At the top, show the ship's name and its type
(the blueprint it was built from). Close the panel when the map stops following
that ship, even if the ship remains selected.

Below that, group the ship's installed modules by module type. Show one
collapsible block per type, headed by that type. Every type block starts
collapsed and can be expanded or collapsed independently.

Inside an expanded type block, show one block per installed module of that
type, headed by the module's name. Leave the contents of every module block
empty for now; module-specific details are not supported yet.

## Acceptance

- Selecting a player ship opens its panel on the right. The panel shows that
  ship's name and blueprint type, and the ship list remains usable.
- Each installed module appears once under its module type. Types without
  installed modules have no block.
- Every type block is initially collapsed. Opening one reveals the names of
  its modules without changing the state of the other type blocks, and it can
  be collapsed again.
- Each module has its own block with its name as the heading and no content
  beneath that heading, regardless of module type.
- Selecting another ship shows that ship's information and installed modules.
  The panel closes when the map stops following the ship, even if it remains
  selected. Clearing the selection or removing the focused ship also closes it.
- When modules are attached to or removed from the focused ship, its panel
  reflects the current set of installed modules.
