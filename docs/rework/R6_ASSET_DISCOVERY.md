# R6 automatic project asset discovery

2026-09-12 (cache recovery updated September 13)

## Source assets and disposable cache

`Assets/` contains the originals. `.pipeframe/cache/<asset-id>/` contains generated
runtime images, compiled payloads and previews; scenes/materials reference stable asset
IDs rather than hardcoded cache paths. A texture's normalized `image.png` may have a
different filename/format from its source, such as `Assets/Textures/ant_background_1K.bmp`.

Opening a project or the Assets panel now checks existing records for importer-version
changes and missing cache/preview/thumbnail files, including decoded texture `image.png`.
It regenerates incomplete/outdated output from the source while preserving IDs and
advancing the asset revision. Healthy caches are left alone. This fixes old texture
importer v3 records incorrectly remaining Ready when v4's decoded image was absent.
Source-content edits still use Reimport; this is not a filesystem watcher. If the original
source itself is missing or invalid, cache reconstruction cannot replace that source.

The Ant cache was regenerated through AssetDatabase from its original Assets files.
Regression coverage includes missing decoded image, deleted cache directory, old importer
version with existing output, stable IDs and no repeated rebuild of healthy caches.
Affected AssetDatabase, VisualAsset, WorkbenchUI and ProjectManager checks pass **4/4 in
Debug and 4/4 in Release**; [evidence](evidence/asset-cache-recovery/release-tests.txt).

## Original discovery implementation

Fixed the missing discovery step: AssetDatabase previously loaded only assets.db,
so existing files under Ant's Assets directory did not appear until manually imported.

AssetDatabase::Open now discovers supported unindexed files recursively under Assets.
Workbench also calls DiscoverProjectAssets when opening the Assets panel, then refreshes
its existing declarative presenter. No project/plugin registration code is required.
Files are imported in place, using built-in extension importers, sorted for deterministic
initial indexing. Existing records, IDs and revisions are preserved. Symlinks are skipped.
Unsupported files (including fonts with no importer yet) are not indexed. Individual
import failures remain visible in operation diagnostics and do not prevent project open.

This is a synchronous discovery scan at project/panel open, not a filesystem watcher.
Existing indexed files are not automatically reimported when modified; use Reimport.
External renames, missing-file reconciliation and background watching remain future work.
Large initial scans can block opening the project/panel; background import is still open.
The browser currently shows asset rows; generated thumbnails do not imply a thumbnail
palette or preview pane has been implemented.

Validation: Debug SimulationWorkbench and AssetDatabaseTests built. Regression tests
cover recursive discovery, ignored unsupported files, stable IDs and revisions, discovery
of newly added files, reopen persistence, and decoding all actual Ant texture files into
an isolated temporary project with ready records and thumbnail outputs.

## Asset categories

The cycling type filter is replaced by directly selectable All, Textures, Audio,
Shaders, Materials, Models, Scenes, Parts, Prefabs and Maps tabs. Maps filters
Tilemap assets; PNG source maps remain textures. Tabs use the shared declarative
View/Wrap/Button controls, indicate selection, and retain the search filter.
An empty category displays an explicit empty result message.

Assets now occupies the full right tools dock while open; closing it restores
Hierarchy and Inspector. This avoids squeezing category controls, asset rows and
actions into the former Inspector-only area. The initial smaller layout failed
assignment/drag checks; the full-height layout passed WorkbenchUIAcceptance (1/1,
4.25 seconds), including direct category clicks and existing assignment/drag checks.
Debug SimulationWorkbench and WorkbenchUITests builds passed.

## Scroll routing correction — 2026-09-13

Nested asset lists no longer consume wheel events when their content fits or they
have reached the requested edge. The shared ScrollPanel bubbles those events to
the surrounding map/material/tileset page, including over empty list space. Inner
lists with remaining overflow still scroll first. Inspector padding and gaps are
covered through WorkbenchUIAcceptance. MountedViewRegression reproduces the short
nested-list failure and covers both edges and scrolling over list buttons.
MountedViewRegression, UIFrameworkRegression and WorkbenchUIAcceptance passed in
Debug and Release (3/3 each). This was a focused regression run.
