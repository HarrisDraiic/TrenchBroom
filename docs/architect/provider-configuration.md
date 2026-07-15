# Provider configuration

The current installable vertical slice intentionally exposes one provider:
`deterministic mock`. It is built into the bundled C++ runtime, needs no account or
credential, performs no network access, and supports only dimensioned rectangular-room
planning.

The AI Architect panel reports the active provider and fixed scale assumption. There is
no provider settings screen yet and no environment file is required or supported.

## Current workflow

1. Open or create a map.
2. Open the AI Architect dock if it is hidden.
3. Optionally select a draft profile. The current mock recognizes only a bounded line
   such as `Room wall thickness: 0.5 metres` in its design language.
4. Enter room width, depth, and height in metres/meters or feet.
5. Select Plan Room, review the assumptions and provenance, explicitly enable write
   access, and select Apply
   Blueprint.

The mock planner uses the editor-supplied current material and 32 map units per metre. It
snaps dimensions and thickness to an 8-unit grid. Without a supported active-profile
rule it preserves the default 0.25-metre shell thickness; with the current directive it
changes wall thickness only.

## Real-provider design requirements

Real provider support is a later milestone. Configuration must be available through the
application UI without editing source, environment files, or JSON by hand. Non-secret
fields may include provider type, model, endpoint allowlist, timeout, and cost/usage
limits. Secret tokens must be stored in the operating-system credential store under the
distinct Architect application identity, never in ordinary preferences, maps, profiles,
logs, crash reports, source control, or packages.

Provider responses must remain semantic blueprints. A provider must never receive a tool
for arbitrary C++ execution, shell execution, filesystem access, unrestricted URL
fetching, or raw brush-plane mutation. The deterministic editor layer remains the only
map writer.

Until this workflow and its authentication, redaction, timeout, cancellation, and error
tests exist, the provider selector must not imply that network models are available.
