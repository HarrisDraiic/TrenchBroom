# Architectural profiles

A fresh TrenchBroom Architect installation contains zero profiles. The current room
vertical slice creates the isolated `Architect/Profiles` data location but does not
create, load, select, or modify profile files. Room generation works without a profile.

## Intended model

A profile is a versioned architectural design system, not a saved prompt. User-global
and project-local profiles will contain human-inspectable design language, proportions,
composition rules, material mappings, negative rules, validation rules, references, and
asset metadata. Explicitly named profiles will override the active profile for one
request without silently changing the permanent selection.

Name resolution is planned in this order: exact display name, alias, normalized slug,
then unique close match. Ambiguous matches must ask for clarification.

## Planned lifecycle

The first profile milestone must support creating a draft through conversation, saving
it explicitly, listing it, selecting and clearing it, renaming or archiving it, and
preserving versions. The runtime must never save a successful map component into a
profile automatically. Global data must not be overwritten by a project-local profile
without a clear user choice.

Import and storage code must validate schema versions and keep canonical paths inside
the selected profile root. Archive extraction must reject traversal, links, absolute
paths, oversized input, and unexpected executable or credential material.

## Compatibility

Profile metadata may be referenced non-invasively from normal maps, but the map remains
an ordinary TrenchBroom-compatible `.map` file. Updating a profile must never silently
rewrite geometry already present in a map.

Profile creation, selection, natural-language lookup, and guided generation are not
implemented in the current package and remain acceptance gaps for the first usable
release.
