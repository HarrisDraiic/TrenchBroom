# Architect bridge protocol

The current vertical slice uses a private child-process standard-input/standard-output
channel. It does not open a TCP port, local socket, or public automation endpoint.

## Lifecycle and transport

`ui::ArchitectPanel` starts the fixed executable
`TrenchBroomArchitectRuntime.exe` from the application directory with `QProcess`.
It passes one fixed `--profile-root` under Architect's isolated application data, does
not invoke a shell, and never accepts a profile path through the protocol. Requests and
responses are newline-delimited UTF-8 JSON. Only one operation is active at a time; Stop
terminates and restarts the child. Closing the panel's owning window also terminates the
child.

The protocol version is `architect/1`. Each request has this envelope:

```json
{"protocol":"architect/1","id":"request-1","method":"plan.room","params":{}}
```

A successful response repeats the protocol and request ID and contains `result`. A
normal failure contains `error.code` and a user-safe `error.message`; it does not contain
a stack trace or sensitive path.

## Current methods

- `runtime.status` returns the runtime name, `provider: mock`,
  `transport: child_process_stdio`, `external_access: false`, and whether the fixed
  profile store is enabled.
- `profiles.list` returns all valid drafts plus the active profile ID.
- `profiles.create_draft` explicitly saves a display name, design language, and aliases.
- `profiles.resolve` resolves a display name, alias, or normalized slug.
- `profiles.set_active` and `profiles.clear_active` persist the selector state.
- `plan.room` accepts `prompt`, `units_per_metre`, and `material`, then returns one
  validated `room/1` blueprint. It never writes to a map.

Unknown methods and protocol versions return stable structured errors. Request IDs must
be non-empty strings of at most 128 characters. A request or buffered response is
limited to 64 KiB, a prompt is limited to 8 KiB, and the editor cancels a request after
10 seconds.

## Mutation boundary

The runtime never edits a map. Its only persistent mutation is the bounded profile
lifecycle inside the fixed profile root. The editor parses room responses again with
`roomBlueprintFromJson`, keeps the blueprint pending for review, and enables Apply only
when the user explicitly checks the write-access box. Geometry creation then runs
synchronously on the Qt UI thread through `mdl::createArchitectRoom` and the existing
map command history.

## External clients

There is no endpoint descriptor, listener, or external-client mode in this milestone,
so there is no remotely reachable session to authenticate. A future local automation
endpoint must add high-entropy per-session authentication, freshness checks, client and
concurrency limits, and an explicit disabled-by-default setting before it is exposed.
The private child-process protocol must not be repurposed as an unauthenticated socket
protocol.
