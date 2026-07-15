# Installing TrenchBroom Architect

The current alpha delivery format is a self-contained portable ZIP. It does not overwrite
or require a stock TrenchBroom installation.

## Obtain the package

1. Open the GitHub Actions run for the Architect draft pull request.
2. Download the `windows-2022` artifact.
3. Verify that it contains a ZIP named like
   `TrenchBroomArchitect-Win64-AMD64-<version>-Release.zip` and the matching
   `.zip.md5` checksum.
4. Verify the checksum before extracting the archive.

CI package verification requires `TrenchBroomArchitect.exe`,
`TrenchBroomArchitectRuntime.exe`, and `LICENSE.txt`; rejects the stock
`TrenchBroom.exe` name; and rejects common credential, key, and live-session descriptor
filenames.

A locally built and clean-extraction-tested development ZIP exists in the build
workspace. It is not downloadable from GitHub. The remotely reproducible package is not
available until the draft PR's Windows workflow creates and passes a run.

## Install or extract

1. Create a new folder, for example `C:\Tools\TrenchBroom Architect Alpha`.
2. Extract the complete ZIP into that folder.
3. Start `TrenchBroomArchitect.exe`.

Open or create a map and show the dockable AI Architect panel. Enter a request with
explicit width, depth, and height, select Plan Room, review the blueprint summary, check
write access, and select Apply Blueprint. The current mock provider needs no credential
and performs no network request. A successful room is one normal undoable command.

To save a draft profile, enter design-language guidance in the same prompt box, choose
**Create Draft Profile**, and name it. The profile appears in the selector as **Draft**.
Selecting it or **No profile** persists that choice. To exercise the current bounded
guidance, include a line such as `Room wall thickness: 0.5 metres` in the design language,
select the draft, and plan a new room. The blueprint summary shows the profile UUID,
slug, version, assumption, and snapped wall thickness. Profile actions and planning do
not write to the map; Apply still requires explicit write access.

Keep `TrenchBroomArchitectRuntime.exe` and all DLL/plugin directories beside the main
executable. Copying only the main executable is not an installation.

The ZIP includes Qt, vcpkg-built runtime libraries, game configurations, fonts, shaders,
styles, the manual, and license material. Visual Studio, CMake, Qt, Python, vcpkg, and
other developer tools are not required on the destination machine.

The unsigned alpha may trigger Windows reputation warnings. This does not indicate that
the binary is signed or trusted; inspect the GitHub workflow and checksum before running
it. Signing hooks can be added when a suitable certificate and protected CI secret are
available.

## Portable data

Start the application with `--portable` to keep preferences and Architect data in the
extracted application's `config` area. A fresh profile store contains zero profiles;
the first draft is created only by the explicit panel action. Portable mode does not use
stock TrenchBroom's configuration directory.

Without `--portable`, Qt places data under the distinct `TrenchBroomArchitect` application
namespace. Architect profiles and runtime data use `Architect/Profiles` and
`Architect/Runtime` beneath that application data directory.

## Current limitations

The current package provides the AI Architect panel, bundled mock runtime, semantic room
planning, deterministic room application, normal undo/redo, and a user-global draft
profile lifecycle with one bounded wall-thickness rule and preview provenance. It does
not provide a real-provider configuration screen, project-local/rename/archive/version
profile workflows, profile assets, general design-language interpretation, durable map
provenance, selection-aware placement, or a signed installer. It remains an unsigned
development alpha rather than the first usable release.
