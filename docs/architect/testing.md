# Testing TrenchBroom Architect

Build the narrowest relevant test target before running its Catch2 executable. The
repository's `AGENTS.md` and `BUILD.md` remain authoritative for platform setup.

## Focused local commands

For a configured Release Ninja tree named `build-architect`:

```text
cmake --build build-architect --target ArchitectLibTest
build-architect/lib/ArchitectLib/test/ArchitectLibTest --reporter compact

cmake --build build-architect --target TbMdlLibTest
build-architect/lib/TbMdlLib/test/TbMdlLibTest ArchitectRoomBuilder --reporter compact

cmake --build build-architect --target TbUiLibTest
build-architect/lib/TbUiLib/test/TbUiLibTest *MapWindow* --reporter compact
```

Set `QT_QPA_PLATFORM=offscreen` for headless UI tests where the selected Qt package
includes that platform plugin. Existing UI tests may also need permission to create the
normal per-user log.

## Current automated coverage

- `ArchitectLibTest`: mock planner metric/imperial behavior, deterministic snapped
  dimensions, JSON round trip, malformed requests, protocol mismatch, unknown methods,
  prompt limits, runtime status, empty profile startup, draft creation, schema/path
  validation, display/alias/slug resolution, collision and ambiguity handling, and
  persistent active selection.
- `TbMdlLibTest` / `ArchitectRoomBuilder`: eight-brush room creation, one undo,
  redo, invalid blueprint rejection, world-bounds rejection, and unchanged-map failures.
- `TbUiLibTest` / `MapWindow`: existing window lifecycle behavior with the docked panel
  integration compiled into the editor, plus the isolated Architect data path.
- Runtime smoke: newline-delimited status, exact natural-language room request, and a
  five-step fresh-list/create/select/persist/clear profile sequence.
- Package verifier: branded application, bundled runtime, license, checksum, stock-name
  exclusion, and credential/session filename patterns.
- Clean-package smoke: the extracted runtime starts without development paths and the
  extracted editor remains running through a short launch check.

Compiler success is not a substitute for interactive verification. The current run has
not automated clicking Plan/Apply inside a real map, checking the rendered room, or
manually invoking undo/redo from the packaged UI.

A deep binary-string scan currently fails the no-build-path release check: the main
executable, stripped PDB, and 31 vcpkg DLLs retain local source/PDB paths in Microsoft PE
debug records. The clean package launches, and no credential value was found; broad
searches for words such as `password` produce expected Qt/GPL text and are not secret
evidence. Release hardening must apply reproducible path mapping or strip/sanitize debug
records while preserving the intended crash-symbol workflow, then make this scan pass.

## CI

The Windows workflow reuses the upstream configure/build/test/package pipeline, invokes
the Architect package verifier, and uploads the ZIP and checksum. GitHub Actions must
produce a check suite for the current branch before remote CI can be recorded as passed.
At the time of this milestone, the fork's Actions page has not created a workflow run, so
local results are recorded separately and CI remains blocked/unverified.

## Required next tests

The first usable release still needs direct panel/runtime click-through tests, provider
configuration and credential tests, authenticated external bridge tests if that bridge
is added, project-scale/selection tests, project-local/rename/archive/version profile
tests, profile archive traversal tests, asset lifecycle tests, broader map-format
geometry coverage, and the complete profile-guided end-to-end smoke sequence.
