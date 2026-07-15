# TrenchBroom Architect repository architecture

This document records the source and delivery architecture at the baseline used for
TrenchBroom Architect. It describes the repository as it exists at commit
`3aa6570ae0b59afa191ba1966f4df1d0f03d8c5b`; it is not a proposed replacement
architecture.

TrenchBroom Architect is an unofficial fork. TrenchBroom remains copyright its
respective contributors and is distributed under GPL-3.0-or-later. The fork must retain
the notices in existing source files and `LICENSE.txt`.

## Baseline

| Item | Recorded value |
| --- | --- |
| Fork | `https://github.com/HarrisDraiic/TrenchBroom.git` |
| Upstream | `https://github.com/TrenchBroom/TrenchBroom.git` |
| Base branch | `master` |
| Origin `master` | `3aa6570ae0b59afa191ba1966f4df1d0f03d8c5b` |
| Upstream `master` | `3aa6570ae0b59afa191ba1966f4df1d0f03d8c5b` |
| Upstream stable tag | `v2026.1` at `b8c14a93c6945a389c56ff7bf77e869c16f24895` |
| Architect branch | `feature/trenchbroom-architect` |
| Host | Windows 10.0.26200, x64 |
| Required Qt | Qt 6.10 (from `BUILD.md` and CI) |
| Language/build | C++20 and CMake 3.25 or newer |
| Dependencies | Qt plus the checked-out `vcpkg` submodule |
| Packaging | CPack; ZIP on Windows, ZIP/app bundle on macOS, external AppImage generator on Linux |

The starting worktree was clean. The GitHub identity `HarrisDraiic` has push and admin
permission on the fork. The local host did not expose CMake, Ninja, Pandoc, Qt, or a C++
compiler on `PATH`, so the unmodified local build command could not start. The canonical
baseline build is therefore the pull-request run of the existing GitHub Actions pipeline.
That pipeline provisions the missing tools rather than depending on this host.

The `vcpkg` checkout resolves to `9a5d7860d1e3c0e394495744cecbf0f5cf41306a`.
The Git-for-Windows submodule status helper failed on this host while creating its MSYS
signal pipe (`Win32 error 5`), but `vcpkg` itself is present and its Git HEAD is readable.
CI performs `actions/checkout` with `submodules: recursive` and bootstraps that checkout.

## Build graph

The root `CMakeLists.txt` sets the vcpkg toolchain and overlays, requires C++20, locates
Git and Pandoc, imports the dependency packages, and finds Qt modules `Core`, `Widgets`,
`OpenGL`, `OpenGLWidgets`, `Network`, `Svg`, and `Test`. It then includes `lib/` and
`app/`.

Applications are defined below `app/`:

- `TrenchBroom` is the Qt GUI application in `app/TrenchBroom`.
- `CmdTool` and `DumpShortcuts` are support executables.

Shared targets are below `lib/`. The Architect work is expected to use these existing
boundaries rather than moving editor code into the application executable:

- `TbBaseLib`: results, logging, preferences, and shared infrastructure.
- `TbFsLib`: filesystem abstractions and disk I/O.
- `TbMdlLib`: the map model, brushes, entities, commands, transactions, materials, game
  configuration, parsing, and writing.
- `TbRenderLib`: map and object renderers.
- `TbPreferencesLib`: declared application preferences.
- `TbUiLib`: document/window ownership, actions, inspectors, map views, and Qt UI.
- `UpdateLib`: update HTTP, archive extraction, and update process management.

Libraries with tests define focused Catch2 executables such as `TbMdlLibTest` and
`TbUiLibTest`. Tests must be built before their executables are run.

## Application lifecycle and identity

`app/TrenchBroom/src/Main.cpp` configures OpenGL and Qt application attributes, parses
`--portable` before preferences are loaded, sets Qt's INI settings format, and currently
sets the application name to `TrenchBroom` and organization domain to
`io.github.trenchbroom`. It creates the `PreferenceManager`, constructs `AppController`,
opens command-line maps or the welcome window, and enters the Qt event loop.

These hard-coded identity values are the first branding isolation point. They also feed
Qt standard paths, so Architect must use a distinct application name/domain before any
preferences, logs, profiles, or runtime descriptors are opened. Portable mode redirects
Qt settings to `./config` and `SystemPaths::setPortable()` redirects user data beneath the
application directory.

`ui::SystemPaths` is the repository's path authority. `userDataDirectory()` is derived
from `QStandardPaths::AppDataLocation` in an installed build and from a local data
directory in portable mode. It owns paths for `Preferences.json`, logs, user game
configurations, temporary files, and installed resources. Architect profiles, runtime
state, and endpoint descriptors should be added through this API; they must not use
ad-hoc absolute paths.

## Windows and document ownership

`ui::AppController` owns application-wide services: `mdl::GameManager`, the background
task manager, OpenGL resource managers, update networking, `MapWindowManager`, recent
documents, `ActionManager`, the welcome window, preferences/about dialogs, and update
lifecycle. Its `newDocument()` and `openDocument()` methods create or load documents via
`MapWindowManager` after resolving the game and map format.

`ui::MapWindowManager` owns the open map windows and is the supported route to the top
active window. Windows use an SDI arrangement on Windows and the repository's existing
platform behavior elsewhere.

`ui::MapWindow` is the `QMainWindow`-based editor shell. It owns a `MapDocument`, the
central map view, inspectors, toolbars, status widgets, menus, and action wiring. It is
the correct integration point for a per-document Architect panel. The current layout is
built directly in `MapWindow`; there is no existing general dock-widget subsystem, so a
dock must be introduced deliberately and its state persisted with the existing
`WidgetState`/`QSettings` conventions.

`ui::MapDocument` adapts the model to the UI. It exposes the active `mdl::Map`, editor
selection, current material, game and material loading, document modification state,
views, autosave, and notifications. It must remain the UI-facing gateway for Architect
operations; session identifiers must never expose addresses of model nodes.

`mdl::Map` is the mutation and history authority. It owns the world node, editor
context, selection, command processor, grid, current layer/group, game state, map format,
world bounds, entity definitions, and material collections. Its public operations add
and remove nodes, select and deselect nodes, set properties and materials, and perform
undo/redo through the existing command processor.

## Deterministic geometry and transactions

`mdl::BrushBuilder` creates convex brushes for the active `mdl::MapFormat` within a
supplied world-bounds box. It supports cuboids and other deterministic primitives and
returns `Result` failures instead of inserting invalid geometry. Architect geometry must
be produced with this builder (or repository generators built on it), not with model-made
brush plane text.

`mdl::Transaction` and `mdl::TransactionScope` wrap `mdl::Map` transactions. A
transaction commits the existing command sequence as a single undo step and cancels on
failure. `mdl::Map::undoCommand()` and `redoCommand()` use the existing
`CommandProcessor`. Architect stages must use meaningful transaction names and this
history; no second undo stack is permitted.

The initial mutation path is therefore:

1. Parse and validate a bounded semantic request away from the model.
2. On the Qt application thread, resolve the active `MapWindow` and `MapDocument`.
3. Read map format, world bounds, grid, selection, and current material.
4. Build every brush into temporary owned nodes with `mdl::BrushBuilder`.
5. Start one `mdl::Transaction` for the logical stage.
6. Insert the nodes through `mdl::Map` operations.
7. Commit only after all insertions succeed; otherwise cancel and return a structured
   error.

## Game, entities, materials, and map compatibility

`mdl::GameManager` loads bundled and user game configurations. `mdl::GameConfig` and its
parsers define supported map formats, filesystem packages, entity definition files, and
compilation tools. `mdl::Map` loads entity definitions and material collections for the
active game and exposes them to `MapDocument` and inspectors.

Normal `.map` files remain the source of truth. Optional Architect provenance must use
non-invasive editor metadata or ordinary entity properties that the active format can
preserve; profile changes must never silently rewrite existing geometry.

## Rendering and capture

`TbRenderLib` contains `render::MapRenderer`, `BrushRenderer`, entity renderers, and the
OpenGL resource layer. `MapWindow` owns the map-view hierarchy, including the
switchable/four-pane views and cameras. There is no established general Architect
capture API at baseline. Viewport and asset capture should be added through the map view
and renderer, with an offscreen test path where supported, and geometric validation must
not be reported as visual review.

## Actions, preferences, and background work

`ui::ActionManager` declares editor actions and their context predicates. `MapWindow`
connects menus and actions to document methods. The Architect panel action should follow
this system so shortcut preferences and enablement stay consistent.

`PreferenceManager`, `TbPreferencesLib`, `QPreferenceStore`, and `QSettings` cover typed
preferences, JSON persistence, recent files, window state, and small Qt-specific state.
Provider secrets must not be added to ordinary preferences. A provider configuration may
store non-secret endpoint/model/timeout fields there, while credentials require the
operating-system credential store or an explicitly documented fallback.

`AppController` owns a `kdl::task_manager` for background work. All model mutations,
window access, and Qt widget updates still have to be dispatched to the Qt application
thread. The compilation and update subsystems demonstrate bounded `QProcess` usage, but
Architect must launch only its packaged runtime executable with a fixed argument schema;
prompts must never become shell commands.

## IPC reconnaissance and security boundary

Qt Network is already a project dependency for updater HTTP, but the editor contains no
existing `QLocalServer`, `QLocalSocket`, `QTcpServer`, or `QTcpSocket` service to extend.
The narrowest cross-platform bridge should therefore use Qt local IPC and introduce a
small, versioned protocol rather than opening a public network listener.

The editor side is responsible for high-entropy per-session authentication, descriptor
freshness, request-size and concurrency limits, timeouts, explicit write enablement,
method allowlisting, stable error codes, and Qt-thread dispatch. The bridge must not
accept arbitrary map text, filesystem paths, URLs, commands, or executable code. It must
be disabled by default for external clients and shut down before its owning application
objects.

## CI and packaging

`.github/workflows/ci.yml` runs reusable Linux, macOS, and Windows workflows on pull
requests, on pushes to `master`, and for tags. All workflows recursively check out
submodules. The Windows workflow installs Qt 6.10, bootstraps vcpkg, installs ccache and
Pandoc, runs `CI-windows.bat`, and uploads the generated ZIP and MD5 checksum.

`CI-windows.bat` configures a C++20 Release build with Visual Studio 2022's toolchain and
Ninja, builds all targets with warnings as errors, runs the full CTest suite, invokes
CPack, and generates the checksum. `app/TrenchBroom/CMakeLists.txt` deploys Qt and runtime
DLLs, installs resources and the update scripts, and configures a flat portable ZIP.

The first Architect package should extend this existing install/component graph. It must
contain the complete Architect runtime, use distinct artifact and executable identity,
contain no credentials or live session descriptors, and remain extractable alongside
stock TrenchBroom. Signing remains a release concern; unsigned alpha packages must be
labelled accurately.

## Implemented vertical slice

- `ArchitectLib` owns stable errors, `architect/1`, strict `room/1` serialization, and
  the deterministic mock planner, including validated optional profile provenance.
- `ArchitectRuntime` is a bundled C++ line-oriented child process with no listener,
  external access, shell, arbitrary URL, or caller-selected filesystem method. Its fixed
  profile store supplies read-only active UUID/version snapshots and one explicit mock
  wall-thickness rule.
- `TbMdlLib` owns `createArchitectRoom`, which preflights and constructs eight cuboids
  with `BrushBuilder` and commits them through one named map command.
- `TbUiLib` owns the docked `ArchitectPanel`, runtime supervision, response validation,
  explicit write confirmation, timeout/cancellation, and UI-thread Apply action.
- CMake installs the main and runtime executables together. The Windows verifier requires
  both, the license, and checksum and rejects stock naming and credential/session files.

## Current verification status

- Passed locally: native Release configuration; `ArchitectLibTest` (136 assertions);
  focused `ArchitectRoomBuilder` test (16 assertions); focused Architect data-path and
  `MapWindow` tests (21 assertions); complete application/runtime build; and a live
  create/select/profile-guided-plan/clear runtime smoke with wall/slab thickness and
  UUID/slug/version assertions. CPack ZIP and MD5 generation, package-content
  verification, packaged runtime query, and a seven-second clean extracted-application
  launch are also validated.
- The local package is unsigned development evidence, not a published release artifact.
- Failed release-hardening check: PE/PDB debug records in the main executable, stripped
  PDB, and 31 vcpkg DLLs retain local build paths. Path mapping or safe debug-record
  stripping is required before a release artifact can claim zero build-tree paths.
- GitHub Actions remains unverified because the fork has produced no workflow/check run
  for the draft PR. Remote reproducibility and downloadable workflow artifacts remain
  blocked until Actions is enabled or permitted to run.
- Manual Plan/Apply interaction, rendered-room inspection, packaged UI undo/redo, and
  clean-machine testing beyond the extracted launch smoke remain pending.

