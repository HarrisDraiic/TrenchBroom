# Remote development

The Architect branch is developed in the user-owned
`HarrisDraiic/TrenchBroom` fork and tracks official
`TrenchBroom/TrenchBroom` as `upstream`. Development occurs on
`feature/trenchbroom-architect`; the synchronized base is
`3aa6570ae0b59afa191ba1966f4df1d0f03d8c5b`.

## Reproducible setup

Follow root `AGENTS.md` and `BUILD.md`. Recursively initialize submodules, use CMake
3.25 or newer, a supported C++20 compiler, Qt 6.10, Pandoc, and the pinned vcpkg
submodule. Build directories and downloaded developer tools are not source artifacts.

On Windows, configure a Release Ninja tree from a Visual Studio 2022 x64 developer
environment with the vcpkg toolchain and Qt prefix. Build the narrowest test target
first. Enable `windeployqt` for the final application build, then use CPack and the
generated checksum script.

## Repository workflow

1. Fetch `origin` and `upstream` without rewriting `master`.
2. Confirm the base SHA and submodule state.
3. Make focused changes on the Architect feature branch.
4. Format only touched C++ files and run focused tests.
5. Build and verify the portable package for packaging changes.
6. Commit coherent milestones, push normally, and update the central draft PR.
7. Record failed, blocked, manual, and credential-dependent checks honestly.

Do not commit build trees, Qt/vcpkg downloads, endpoint descriptors, credentials, private
profiles, logs, or extracted smoke-test packages.

## GitHub Actions

The existing Windows workflow recursively checks out submodules, installs Qt, restores
or builds vcpkg dependencies, configures and builds, runs tests, packages, generates a
checksum, verifies Architect package contents, and uploads artifacts. Linux and macOS
remain compatibility targets.

At this milestone the fork's GitHub Actions page has not created a workflow run for the
draft PR. Local native Windows validation is complete, but it does not replace a remote
check suite. The smallest corrective external step is for Actions to be enabled or a
workflow run to be permitted on the fork; after that, rerun the branch workflow and
retain its artifact and logs in the PR.
