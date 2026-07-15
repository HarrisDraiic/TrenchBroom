# Migrating from TrenchBroom

TrenchBroom Architect intentionally starts with an independent configuration namespace.
It does not silently read, move, rewrite, or delete stock TrenchBroom settings.

Existing `.map` files remain compatible and can be opened normally. Game installations,
custom game configurations, material paths, keyboard preferences, view preferences, and
recent projects are not imported by the current milestone.

## Planned import behavior

The import workflow will be explicit and copy-only:

1. Locate the stock settings directory using stock TrenchBroom's platform conventions.
2. Show the exact source and Architect destination.
3. Let the user choose compatible categories.
4. Validate every source file and value before copying.
5. Back up an existing Architect destination before replacement.
6. Report imported, skipped, and incompatible items without deleting the source.

Recent-document entries will be imported only when paths remain valid. Provider
credentials, Architect profiles, runtime session descriptors, and private service data
will never be inferred from stock settings.

## Manual migration

Until the import workflow exists, open maps directly and configure game paths in the
Architect application. Do not copy an entire stock settings directory over the Architect
directory: it defeats side-by-side isolation and may copy incompatible update or window
state.
