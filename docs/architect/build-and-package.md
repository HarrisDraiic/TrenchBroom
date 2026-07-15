# Building and packaging TrenchBroom Architect

The fork reuses TrenchBroom's CMake, vcpkg, Qt, CTest, and CPack pipeline. It does not
maintain a second build system.

## Supported build inputs

- CMake 3.25 or newer.
- A C++20 compiler supported by upstream TrenchBroom.
- Qt 6.10.
- Pandoc.
- The recursively initialized `vcpkg` submodule.

See the repository `BUILD.md` for platform-specific developer setup. The current remote
workspace does not expose CMake, Qt, Pandoc, Ninja, or a usable C++ compiler, so local
configuration cannot be used as release evidence.

## Windows CI path

The pull-request workflow performs these material operations:

1. Recursively checks out submodules.
2. Installs Qt 6.10.
3. Bootstraps vcpkg and restores binary/ccache inputs.
4. Runs `CI-windows.bat` with warnings as errors.
5. Configures a Release Ninja build with Visual Studio 2022.
6. Builds all configured targets.
7. Runs CTest with failure output.
8. Runs CPack and checksum generation.
9. Runs `.github/scripts/verify-architect-package.ps1`.
10. Uploads the ZIP and checksum as the `windows-2022` artifact.

The expected executable is `TrenchBroomArchitect.exe`; the target's internal CMake name
remains `TrenchBroom` to avoid unrelated build-graph churn. The package filename begins
with `TrenchBroomArchitect-`.

## Focused verification

For a configured developer build, build the relevant test target before running it:

```text
cmake --build cmakebuild --target TbUiLibTest
ctest --test-dir cmakebuild --output-on-failure -R TbUiLibTest
```

The SystemPaths test verifies that Architect profile and runtime locations remain beneath
the isolated application data directory. The Windows packaging check verifies executable
identity, license inclusion, checksum creation, and credential/session filename bans.

## Signing

Alpha artifacts are unsigned unless a signing step and protected certificate secret are
present in CI. Lack of signing material does not block an accurately labelled development
ZIP. Never commit a certificate, private key, token, or password to the repository or
include it in an artifact.
