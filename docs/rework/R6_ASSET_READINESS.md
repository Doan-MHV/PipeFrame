# R6 asset readiness audit

**Package-2 implementation supersedes the historical gaps below.** See
[R6_VISUAL_ASSETS.md](R6_VISUAL_ASSETS.md) for material/tileset schemas and editors,
textured runtime binding, typed property assignment, asynchronous import/reimport,
resize/resample undo, and acceptance evidence. The sections below retain the earlier
audit as history; they are not the current remaining-work checklist.

September 12, 2026. Source/test inspection; no new runtime test was run for this audit.
Last recorded AssetDatabaseRegression and WorkbenchUIAcceptance passed in the
r6-presenters suite. Those passes do not establish tilemap/material readiness.

## Existing working foundation

AssetBrowserPanel describes searchable/filterable asset rows and import/assign/reimport
controls using the neutral ViewPanel API. Workbench wires import to a native file picker
and AssetDatabase::ImportNow. Database tests cover stable IDs, cache records, reimport
revision/hash changes, move/repair, dependencies, search, persistence and cooperative
operation cancellation. Some of those database operations have no equivalent browser UI.
Native Workbench tests cover asset row selection, assignment and viewport-drop assignment,
undo/reference handling and missing-reference checks.

## Texture importer progress — September 12, 2026

Implemented texture importer version 3 using the neutral `ImageData` service. Textures
must decode successfully before cache writes. Texture preview artifacts are now real PNGs:
preview maximum dimension 512, thumbnail maximum dimension 128, preserved aspect ratio,
no upscaling, nearest-neighbour sampling and retained alpha. The existing compiled
container, stable IDs and database format remain unchanged. Existing imports acquire
these artifacts on reimport; this is not an automatic cache migration.

Verification: Debug AssetDatabaseRegression and native WorkbenchUIAcceptance **2/2 passed**
(5.92 seconds). Dependency lint passed. Real PNG fixtures replace fake header-only inputs.
Tests check dimensions, colour/alpha, small-image sizing, changed-source reimport,
corrupt-payload rejection and retention of the prior thumbnail after decode failure.
This does not establish atomic recovery from disk-write failures or cancellation.

Browser image display and image dimensions/settings UI are still outstanding. No new
SFML dependency was introduced into Ant or public authoring headers; decoding stays
behind the existing backend implementation. The asset regression now links that decoder.

## Remaining limitations

- Non-texture built-in importers still write text .pfpreview artifacts. Material previews
  are not rendered. Browser rows remain text even though texture PNG artifacts now exist.
- Material is a category/import extension; the generic importer checks nonempty content
  but does not implement a material schema, material editor or usable shader/texture graph.
- Workbench calls ImportNow/Reimport synchronously from UI callbacks. Database cooperative
  cancellation tests do not prove an interactive CANCEL button during a blocking import.
- Generic ASSIGN/viewport drop chooses the selected object's first editable AssetReference
  property. It is not a complete explicit-target/type-compatible texture/material workflow,
  and ordinary texture drops do not automatically create rendered ground objects.
- Tilemap data, plain-color tile definitions and validated .pftilemap imports now exist
  (see R6_TILEMAP_FOUNDATION.md). Atlas slicing and editor tilemap creation/painting UI
  remain unimplemented.

## Required R6 package-2 acceptance before painting integration

1. Decode real texture files, reject corrupt payloads, expose dimensions/settings and show
   actual thumbnails/previews; use neutral engine resource/asset services.
2. Create/edit/save validated visual materials and tilesets (atlas regions, margins/spacing,
   tile IDs, UV/tint/filtering) with stable dependencies and missing-resource feedback.
3. Resolve assigned stable asset IDs into usable runtime resources. Choose an explicit
   compatible field or creation operation; do not silently pick an unrelated asset field.
4. Keep import/reimport work off the UI thread with safe main-thread publication, visible
   progress and meaningful cancellation. Respect render-resource thread/context ownership.
5. Author a ground/tilemap from a blank project using an actual imported image; render it,
   reimport a changed source, undo/save/reopen and verify the result in the viewport.

Reuse the current database, not a second asset manager. These requirements supplement
R6_ENVIRONMENT_AUTHORING.md and R6_EXECUTION_PLAN.md; the remaining acceptance items are still open.
