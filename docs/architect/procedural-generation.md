# Deterministic procedural generation

The first generator is `mdl::createArchitectRoom` in `TbMdlLib`. It consumes a validated
`architect::RoomBlueprint` and the active `mdl::Map`; it does not parse natural language
and does not call a provider.

## Room construction

The generator validates finite positive dimensions, the supported south doorway, doorway
containment, and the complete outer room bounds. It then uses the active map format,
world bounds, default face attributes, current material, and `mdl::BrushBuilder` to build
eight convex cuboids:

1. floor;
2. ceiling;
3. north wall;
4. west wall;
5. east wall;
6. south wall left of the doorway;
7. south wall right of the doorway;
8. south lintel above the doorway.

Brushes remain temporary until every cuboid succeeds. They are then added through the
existing `mdl::addNodes` command named `Architect: Create Room`. The existing command
processor supplies undo and redo; there is no Architect-specific undo stack.

## Failure behavior

Invalid dimensions, unsupported doorway placement, world-bound violations, brush
construction failures, and command failures return stable `architect::Error` values.
Validation occurs before mutation, and tests verify invalid/world-bound requests leave
the map unchanged.

## Current limits

The origin is fixed by the mock planner, the scale is fixed at 32 units per metre, the
doorway is rectangular and centered on the south wall, and one material is used for the
entire shell. Selection-aware placement, project-scale inspection, missing-material
diagnostics, map-format matrices, staged structures, corridors, stairs, arches, towers,
roofs, and profile assets are planned.

New generators should follow the same pattern: semantic input, strict preflight,
temporary construction, meaningful logical transactions, deterministic validation, and
focused undo/redo tests.
