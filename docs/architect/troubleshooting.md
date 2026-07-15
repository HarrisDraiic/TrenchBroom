# Troubleshooting

## Runtime unavailable

Use a complete package and keep `TrenchBroomArchitectRuntime.exe` beside
`TrenchBroomArchitect.exe`. Do not copy only the main executable. If the package was
moved, extract the entire ZIP again and verify its checksum.

## Plan Room reports invalid argument

The mock provider requires explicit width, depth, and height with metric or imperial
units, for example: ?Create a stone room 12 metres wide, 10 metres deep and 5 metres
tall.? Dimensions must be within the current mock limits and the height must be at least
2.2 metres.

## Apply Blueprint is disabled

A successful plan must exist and ?Enable map write access for Apply Blueprint? must be
checked. Planning never changes the map. The checkbox intentionally resets after a
successful apply.

## World bounds exceeded

The complete floor, ceiling, walls, and thickness must fit the active map's world bounds.
The current mock planner places the room at the origin and does not yet place relative to
selection. Use a compatible map/game configuration or request a smaller room.

## Missing DLL or platform plugin

This indicates an incomplete development-tree copy or package. Use the generated
portable ZIP, which contains Qt and vcpkg runtime dependencies. Visual Studio, Qt, CMake,
and Python are not destination-machine prerequisites.

## Undo

A successful room is one command named `Architect: Create Room`. Use the normal editor
Undo and Redo actions. If Apply reports a structured error, no partial room should have
been inserted.

## Logs and sensitive data

Installed mode uses the separate `TrenchBroomArchitect` application namespace; portable
mode uses the extracted folder's configuration area. The mock milestone stores no
provider credential. Do not post private maps or future provider logs publicly when
reporting a problem.
