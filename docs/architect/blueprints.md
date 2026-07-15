# Semantic blueprints

A blueprint describes architectural intent in bounded data. The model or mock provider
chooses what should exist; deterministic editor code decides how to create valid map
geometry.

## Current `room/1` schema

The current runtime returns this shape inside a successful `plan.room` result:

```json
{
  "schema": "room/1",
  "id": "mock-room-v1",
  "units_per_metre": 32,
  "interior_width": 384,
  "interior_depth": 320,
  "interior_height": 160,
  "wall_thickness": 8,
  "floor_thickness": 8,
  "ceiling_thickness": 8,
  "origin": [0, 0, 0],
  "material": "__TB_empty",
  "doorway": {
    "wall": "south",
    "width": 40,
    "height": 72,
    "center_offset": 0
  },
  "scale_assumption": "Mock provider used the editor-supplied units-per-metre scale."
}
```

Dimensions are map units after deterministic grid snapping. All thickness and dimension
values must be positive and finite, the origin must have three finite values, the
material must be non-empty and bounded, and the doorway must have a supported wall and
valid dimensions. The first geometry generator additionally requires a south doorway
fully contained by the wall and an outer shell contained by active world bounds.

## Planning and execution

The panel sends 32 units per metre and the active map material. The mock parser accepts
metric or imperial width, depth, and height, restricts requested dimensions to 2?100
metres with a minimum 2.2-metre height, and snaps the result to an 8-unit grid. Planning
does not change the map.

The editor parses the returned JSON again. Apply converts the blueprint into eight
temporary cuboids: floor, ceiling, north/east/west walls, and three south-wall pieces
around the doorway. Only after all brushes are valid are they inserted as one undoable
command.

Blueprint IDs currently provide descriptive provenance only; persistence and map
metadata are not implemented. Later schemas should add staged components, dependencies,
profile/asset version references, assumptions, estimated brush counts, and validation
results without allowing arbitrary brush-plane or code payloads.
