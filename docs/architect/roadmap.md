# TrenchBroom Architect roadmap

## Completed milestones

- Isolated product/application/package identity and side-by-side data paths.
- Bundled C++ runtime with private child-process JSON transport.
- Offline deterministic mock provider and semantic `room/1` blueprint.
- Dockable AI Architect panel with explicit write confirmation.
- Eight-brush room with one south doorway and one undoable editor command.
- Focused protocol, planner, geometry, undo/redo, UI-window, package, and clean-launch
  validation.
- Self-contained unsigned Windows portable ZIP and checksum.
- Empty-by-default versioned draft-profile store with human-inspectable JSON/Markdown.
- Explicit profile create/list/select/clear, persistent active selection, and safe
  display-name/alias/slug resolution in the bundled runtime and panel.
- Focused profile schema, path, collision, ambiguity, persistence, and protocol tests.

## Next smallest milestone

Make one deterministic room request consume a constrained, versioned profile snapshot
and record profile provenance without silently mutating the profile or existing map
geometry. Add project-local precedence only with an explicit user choice, then extend
the draft lifecycle with rename, archive, and version history.

## First usable release remainder

- Provider configuration and secure credential storage for a real model provider.
- Authenticated local automation only if external clients are enabled.
- Project-local, collaborative, and multi-version profile workflows.
- One isolated asset workspace with draft/approve/version/revert/insert.
- Profile-guided room generation and provenance.
- Remote GitHub Actions artifact and broader/manual GUI verification.
- Accurate unsigned alpha release notes and installation evidence.

## Later milestones

Expand deterministic generators in coherent stages: selection-aware placement,
corridors, stairs, arches, multi-floor shells, towers, roofs, courtyards, halls, and
validation. The second release target is a staged, gameplay-usable castle blockout.
Procedural cities, arbitrary model-generated brush planes, arbitrary code/shell
execution, cloud profile hosting, and automatic profile mutation remain non-goals.
