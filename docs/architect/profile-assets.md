# Profile assets

Profile assets are planned as independently versioned map-compatible components. No asset
store or asset workspace is included in the current room milestone.

Each asset must be independently viewable, editable, validated, approved or left as a
draft/experimental item, deprecated, reverted, and reused. Opening or editing an asset
must use an isolated document so a failure cannot replace or modify the user's current
map.

An asset record will need a stable UUID, profile and asset versions, semantic tags,
dimensions and scale constraints, material dependencies, connection points, validation
state, and a source map or equivalent inspectable geometry representation. Previews are
derived artifacts and must not replace editable source geometry.

## Required first workflow

1. Create an asset draft through the profile conversation.
2. Open it in an isolated asset workspace.
3. Modify and validate it.
4. Generate a clearly labelled preview.
5. Explicitly approve a version.
6. Insert that exact version into another map through a normal undoable transaction.
7. Verify map-local edits do not change the profile source.

Missing dependencies, invalid bounds, stale versions, and incompatible map formats must
produce structured errors and leave both documents unchanged. Improvised map geometry
may be offered as a future draft, but must never be saved into the profile without
approval.

Asset creation, editing, preview, approval, versioning, and insertion are current release
gaps, not hidden features of the mock provider.
