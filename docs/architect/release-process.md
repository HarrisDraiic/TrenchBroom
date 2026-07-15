# Architect release process

The current deliverable is an unsigned Windows portable alpha ZIP. It reuses the
repository's CMake, CPack, and GitHub Actions paths.

## Prepare

1. Confirm `origin` is the user-owned fork and `upstream` is official TrenchBroom.
2. Confirm the feature branch is based on a synchronized, recorded upstream commit.
3. Recursively initialize submodules and use the Qt and vcpkg versions required by
   `BUILD.md`.
4. Keep generated build trees, session data, credentials, and private profiles untracked.
5. Run formatting and `git diff --check`.

## Validate

Build and run `ArchitectLibTest`, focused `TbMdlLibTest` room tests, and relevant
`TbUiLibTest` tests. Build `TrenchBroom` and `ArchitectRuntime` with deployment
enabled. Run CPack, generate the repository-configured MD5 sidecar, and execute
`.github/scripts/verify-architect-package.ps1`.

Extract into a fresh directory, remove development-tool directories from `PATH`, query
the packaged runtime, and launch the packaged editor briefly. Record manual UI checks
separately from automated checks.

## Publish

Push focused commits to `feature/trenchbroom-architect` and update the existing draft PR
with exact passed, failed, blocked, and manual results. A GitHub Actions ZIP is the
remotely reproducible artifact of record; a local ZIP is development evidence only.
Do not create a release or mark the PR ready while first-release acceptance gaps remain.

The package filename and executable identity come from centralized Architect branding.
The package is unsigned unless protected code-signing credentials and a reviewed signing
step are available. Signing secrets must never be placed in source or artifacts.

## First usable release gate

In addition to the deterministic room slice, the first usable release requires a real
provider configuration workflow or explicitly accepted mock-only scope, profile draft
creation and selection, an isolated editable asset workflow, broader profile-guided
generation beyond the bounded wall rule, durable provenance, remote CI artifacts, and
the documented manual GUI checks. Until then, label packages as
development alpha artifacts.
