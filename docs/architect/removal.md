# Removing TrenchBroom Architect

Removing the portable alpha does not remove or modify stock TrenchBroom.

## Portable mode

1. Close every TrenchBroom Architect window.
2. If any maps or profile drafts are stored under the extracted folder, copy the data you
   want to keep.
3. Delete the extracted TrenchBroom Architect folder.

Portable preferences, logs, profiles, and runtime data are kept with that folder, so the
third step removes them as well.

## Installed-data mode

1. Close every TrenchBroom Architect window and bundled runtime process.
2. Delete the extracted application folder.
3. To remove personal Architect state too, delete only the operating-system application
   data directory belonging to the `TrenchBroomArchitect` Qt application identity.

Do not delete a `TrenchBroom` directory when removing this fork. That directory belongs
to the stock application. If there is any doubt, retain the data and inspect its path and
contents before removing it.

## Maps and projects

Normal `.map` files and project-local files are user documents, not application cache.
They are never removed automatically. Future project-local profile stores will also stay
with the project unless the user deletes them explicitly.

## Credentials

The identity milestone stores no provider credentials. When provider support is added,
removal instructions must name the operating-system credential entry separately because
deleting an application directory may not remove secure credential-store entries.
