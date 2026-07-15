# TrenchBroom Architect overview

TrenchBroom Architect is an unofficial fork of TrenchBroom for collaborative,
AI-assisted architectural level design. It is not affiliated with or endorsed by the
TrenchBroom project.

The application keeps ordinary TrenchBroom-compatible `.map` files as the map source of
truth. AI providers decide what architecture to plan; deterministic editor code is
responsible for producing valid brushes, entities, transactions, and undo history.

## Product boundary

The intended system has four boundaries:

1. The forked C++/Qt editor owns documents, selection, materials, deterministic geometry,
   transactions, undo/redo, rendering, and user confirmation.
2. A packaged Architect runtime owns provider integration, conversation state, semantic
   blueprints, profiles, and coordination. End users will not install a language runtime.
3. A narrow, authenticated local protocol connects the two processes. It is local-only,
   disabled for external clients by default, and has no arbitrary code, shell, URL, or
   filesystem method.
4. User-global and project-local profile stores contain inspectable versioned design
   systems and independently editable assets. A fresh installation contains no profiles.

## Current implementation status

The feature branch now contains an installable deterministic room vertical slice:

- Centralized temporary product naming.
- Distinct executable, package, Qt application identity, settings, logs, bundle ID, and
  Linux desktop identity.
- Side-by-side portable ZIP packaging based on TrenchBroom's existing CPack pipeline.
- Package-content verification for the Windows artifact.
- Stock TrenchBroom auto-updates disabled until the fork has its own compatible update
  channel.
- Dedicated empty locations for Architect profiles and runtime data; no built-in profiles
  are created or shipped.

- A bundled C++ Architect runtime using private child-process standard I/O, a versioned
  bounded JSON protocol, and an offline deterministic mock provider.
- A dockable AI Architect panel that plans without writing, requires explicit write
  confirmation, enforces a timeout, and supports cancellation.
- A semantic `room/1` blueprint and deterministic eight-brush room generator using the
  active map format, material, world bounds, and existing undo/redo history.
- Focused protocol/planner and geometry transaction tests, an offscreen window test, an
  actual Windows ZIP verification, and a clean extracted-package launch smoke test.

This is not the first usable release. Real-provider configuration, collaborative
profiles, independently editable assets, profile-guided generation, remote Actions
evidence, and complete interactive smoke tests remain outstanding.

## Licensing and attribution

Existing copyright notices remain in place. The complete application is distributed
under GPL-3.0-or-later and packages include `LICENSE.txt`. The About view identifies this
repository as an unofficial fork and links to the upstream TrenchBroom repository.

## Delivery priority

Windows 10/11 x64 is the first packaging target, using an unsigned portable ZIP for the
alpha. The same identity configuration is used by macOS and Linux builds so later
packages do not collide with stock TrenchBroom data or executable names.
