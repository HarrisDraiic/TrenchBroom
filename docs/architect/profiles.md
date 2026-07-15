# Architectural profiles

A fresh TrenchBroom Architect installation contains zero profiles. Profiles are created
only when the user explicitly chooses **Create Draft Profile** in the Architect panel.
Room generation works without a profile.

## Current draft workflow

1. Describe the design language in the Architect prompt box.
2. Choose **Create Draft Profile** and provide a display name.
3. The bundled runtime saves a version 1 draft under the isolated
   `Architect/Profiles` data directory.
4. Use the panel's profile selector to activate the draft or return to **No profile**.

The active selection is persisted in `active-profile.json`. Creating or selecting a
profile does not write to the map. Each subsequent room plan reads a validated snapshot
of that exact UUID and version; it never mutates the profile, changes existing geometry,
or silently falls back when the selected draft is missing or malformed.

## Current planning rule

The deterministic mock intentionally recognizes one constrained directive in the active
design language:

```text
Room wall thickness: 0.5 metres
```

Metric and imperial values from 0.1 through 2 metres are accepted and snapped to the
8-unit grid. The rule changes only newly planned wall thickness; floor and ceiling slabs
retain their deterministic default. If no supported directive is present, default
geometry is preserved but the selected UUID, slug, and version are still recorded in the
pending blueprint. A malformed or out-of-range directive returns `invalid_argument`
instead of being ignored. The panel displays both the planning assumption and provenance
before Apply is enabled.

## Stored format

Each profile has a safe normalized directory name and two human-inspectable files:

- `profile.json` uses schema `architect-profile/1` and records a UUID, display name,
  normalized slug, aliases, integer version, and `draft` status.
- `design-language.md` records the display name, draft status, version, and the user's
  design-language text.

Profile JSON is limited to 64 KiB, design-language input to 8 KiB, names and aliases to
128 characters, and aliases to 32 entries. Writes use atomic save files. Listing rejects
unsupported schemas, invalid UUIDs, malformed versions, links, and metadata paths that
do not remain canonically inside the selected root. Active snapshots additionally
require the expected Markdown header, bounded valid UTF-8 content, and a normal
non-linked file contained by the fixed profile root.

Name resolution is deterministic: exact display name, exact alias, then normalized slug.
Display and alias comparisons are case-insensitive. Ambiguous aliases return a stable
`ambiguous_profile_name` error instead of silently choosing a profile.

## Current boundaries

This milestone provides a user-global draft store, explicit create/list/select/clear,
persistent active selection, safe name resolution, and one explicit profile-guided room
rule. It does not yet provide project-local precedence, rename/archive, multi-version
history, imports, close-match lookup, profile assets, or general narrative/composition/
material interpretation. A successful room is never added to or used to modify a
profile automatically.

Future archive import must reject traversal, links, absolute paths, oversized input, and
unexpected executable or credential material. Global data must not be overwritten by a
project-local profile without a clear user choice.

## Compatibility

Profile metadata is stored outside normal maps. The map remains an ordinary
TrenchBroom-compatible `.map` file, and changing the active profile never rewrites
existing geometry. Current provenance exists only in the pending blueprint and panel;
it is not written into the map.
