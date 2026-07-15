# Architect security model

This document separates protections implemented by the deterministic room milestone
from controls required before real providers or external automation are enabled.

## Implemented boundary

- The editor launches one fixed, package-relative runtime executable directly with
  `QProcess`; it does not invoke a shell.
- The runtime uses inherited standard-input/standard-output pipes and exposes no listener
  or network request API. `runtime.status` reports `external_access: false`.
- The runtime has no method for arbitrary code, commands, URLs, or caller-supplied
  filesystem paths. Profile methods are confined to the fixed root passed by the editor.
- Requests are limited to 64 KiB, prompts to 8 KiB, and editor operations to 10 seconds.
  Only one request can be active in a panel.
- Every response must have the expected request ID and protocol. Room blueprint numbers,
  origin, material, schema, doorway, and world bounds are validated before mutation.
- Planning is read-only. Apply remains disabled until the user checks write access and a
  valid pending blueprint exists. The checkbox resets after a successful apply.
- All map mutation occurs from the Qt button handler on the application thread.
- All eight brushes are constructed before insertion and added through one existing map
  command named `Architect: Create Room`. Invalid input and brush-build failures leave
  the map unchanged.
- Runtime stderr is discarded rather than inserted into the transcript. Structured
  client errors contain stable codes and user-safe messages.
- A fresh profile store is empty. Draft creation is an explicit button action; schemas,
  UUIDs, integer versions, normalized slugs, canonical containment, file/link types, and
  size limits are validated. Metadata and active selection use atomic save files.
- Room planning resolves the active UUID only from the fixed store, reads a bounded
  normal UTF-8 design-language file, validates its expected version header, and fails on
  stale or malformed state. The caller cannot inject a profile path. Optional blueprint
  provenance is revalidated as UUID/slug/integer-version data by the editor.
- No provider credential is accepted, stored, logged, committed, or packaged.
- The Windows package check rejects common credential, key, and live-session descriptor
  filenames and requires the branded application, bundled runtime, license, and checksum.

The current child pipe is private to the parent and child processes. Authentication is
therefore not an exposed-client mechanism in this milestone; no socket exists for a
third-party client to connect to.

## Untrusted input

Prompts and runtime JSON are untrusted. The planner accepts only the small `plan.room`
schema, and profile methods accept bounded names, aliases, and design-language text
without accepting a path. The geometry layer does not accept brush planes or raw map
text. The active material name is treated as data passed to `mdl::BrushBuilder`, and all
geometry must fit the active map's world bounds.

Maps, game configurations, materials, entity definitions, profile archives, and
references require their existing parsers plus feature-specific validation as later
milestones begin consuming them.

## Required before expansion

A future external local endpoint requires authenticated, expiring session descriptors,
local-machine binding, stale-descriptor rejection, rate and concurrency limits, and
explicit opt-in. Real providers require operating-system credential storage, log and
crash-report redaction, endpoint allowlisting, and tests for authentication and provider
failures. Project-local profiles and assets require explicit precedence, archive
validation, safe extraction, and confirmation before deletion or replacement. Profile
and asset importers must extend the current schema/version, size, link, and canonical
containment checks to every archive entry.

No external endpoint, real provider, profile importer, or asset importer should be
enabled until those controls and tests are implemented.
