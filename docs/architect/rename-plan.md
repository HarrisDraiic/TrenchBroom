# Product rename plan

`TrenchBroom Architect` is a temporary internal product name. Do not rename the GitHub
repository or published identities before the first usable release passes and the user
explicitly approves the final name.

Most application identity is centralized in `cmake/ArchitectBranding.cmake`, including
display/package names, executable names, Qt organization/application identity, bundle
and Linux IDs, settings/log names, and runtime executable names. A final rename must
still be treated as a migration rather than a text replacement.

## Approval-time checklist

1. Choose the final display name and confirm trademark/domain availability.
2. Decide whether to rename the GitHub repository and preserve redirects.
3. Update central branding values, icons, About attribution, documentation URLs, desktop
   metadata, bundle ID, installer identity, Start-menu label, and release artifact names.
4. Create a settings/profile/runtime migration from the temporary namespace. Copy and
   validate data; do not silently delete the old namespace.
5. Update workflow names, artifact filters, package verification, update-channel URLs,
   signing subjects, crash-report service names, and protected secrets.
6. Document how existing clones update `origin` after a repository rename.
7. Test side-by-side behavior with stock TrenchBroom and with a previous Architect alpha.
8. Publish release notes that retain TrenchBroom copyright, GPL, source availability, and
   unofficial-fork attribution.

Do not reuse stock TrenchBroom's executable, organization domain, settings directory, or
update feed. A repository rename does not itself rename local clones or application data.
