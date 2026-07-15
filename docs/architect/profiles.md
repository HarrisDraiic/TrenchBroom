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
profile does not write to the map, and the deterministic mock room planner does not
apply profile rules yet.

## Stored format

Each profile has a safe normalized directory name and two human-inspectable files:

- `profile.json` uses schema `architect-profile/1` and records a UUID, display name,
  normalized slug, aliases, integer version, and `draft` status.
- `design-language.md` records the display name, draft status, version, and the user's
  design-language text.

Profile JSON is limited to 64 KiB, design-language input to 8 KiB, names and aliases to
128 characters, and aliases to 32 entries. Writes use atomic save files. Listing rejects
unsupported schemas, invalid UUIDs, malformed versions, links, and metadata paths that
do not remain canonically inside the selected root.

Name resolution is deterministic: exact display name, exact alias, then normalized slug.
Display and alias comparisons are case-insensitive. Ambiguous aliases return a stable
`ambiguous_profile_name` error instead of silently choosing a profile.

## Current boundaries

This milestone provides a user-global draft store, explicit create/list/select/clear,
persistent active selection, and safe name resolution. It does not yet provide
project-local precedence, rename/archive, multi-version history, imports, close-match
lookup, profile assets, or profile-guided generation. A successful room is never added
to a profile automatically.

Future archive import must reject traversal, links, absolute paths, oversized input, and
unexpected executable or credential material. Global data must not be overwritten by a
project-local profile without a clear user choice.

## Compatibility

Profile metadata is stored outside normal maps. The map remains an ordinary
TrenchBroom-compatible `.map` file, and changing the active profile never rewrites
existing geometry.
