# R6 Tilemap component and runtime assets

September 12, 2026. Status: imported tilemap assignment/rendering verified; painting UI open.
This checkpoint introduced plugin ABI **7**; the current ABI is **10**. Rebuild generated/external plugins against the current SDK.

## Implemented path

SceneProjectRuntime registers TilemapComponent, and its Playground recipe now attaches
Transform, Playground and Tilemap. TilemapComponent::Schema() exposes a stable AssetReference
with `asset:Tilemap` compatibility metadata and a Visible flag. The standard Inspector
uses this schema; no native project UI or specialized Inspector was added.

Create a Playground in a rebuilt generated project, import a `.pftilemap` through Assets,
and assign it to the Tilemap Asset field. The scene stores the stable ID. Component edits
with typed asset metadata reject missing, non-ready and wrong-type references. Empty
references clear the slot. The existing browser's first-asset-field assignment behavior
remains; a generalized explicit field-target chooser is still outstanding.

ProjectRuntimeHost publishes ProjectSession's existing AssetDatabase through ServiceRegistry.
The database outlives the host/runtime. Reload shares the same service, and unload clears
runtime caches. There is no duplicate asset database or asset manager.

TilemapAssetModule derives RuntimeModule. It resolves the imported compiled cache, validates
the tilemap payload, and caches decoded data/geometry by stable ID and asset revision.
Normal frames do not open the file again. Reimport advances the revision and refreshes
geometry. Missing/failed/wrong-type assets produce diagnostic state and a log message;
no stale painted geometry is rendered for those states. Decode failures require reimport
to retry. This is synchronous first-resolution work, not asynchronous import completion.

SceneProjectRuntime draws cached plain-color cells through Canvas, applying the entity's
Transform. Ground renders first and tilemap layers above it. Empty/failed references leave
the ground fallback visible. Texture-atlas materials and simulation collision integration
are still outstanding. Geometry is transformed per frame; large-map viewport culling and
performance acceptance have not been completed.

## Current bounds limitation

Imported tilemaps retain their own local origin, dimensions and cell size. Playground
bounds now clip rendering without changing the attached asset (R6_PLAYGROUND_BOUNDS.md).
Grid resolution remains independent; set compatible rows/columns/cell size manually. Painting must not ship until target
bounds, crop/resize preview and asset ownership are integrated. Visual clipping is verified; painting/collision bounds and asset resize/crop are still open.

## Verification

Debug engine/Workbench, runtime and generated-project builds passed. Neutral presenter
compile and dependency lint passed. SceneProjectRuntimeAcceptance, WorkbenchUIAcceptance
and ProjectBuildAcceptance: **3/3 passed**, 14.77 seconds.
Evidence: `evidence/r6-tilemap-component/tests.txt`.

Tests resolve the actual imported cache, retain cached data until reimport, refresh the
changed geometry, report missing assets and record actual runtime Canvas submissions
from attached components. Generated project tests assign a tilemap, reject a shader in
that slot and preserve the stable ID after rebuild/reload/save/reopen. Native editor
acceptance includes stale ABI rejection. No new rendered screenshot was taken here.

## Remaining R6 work

Declarative palette/layer/brush UI, viewport painting gestures and previews, scene-level
stroke undo/redo, asset creation/edit ownership, bounds synchronization/cropping, material
and thumbnail display, asynchronous import, Ant component migration, remaining native
host/widget isolation, performance and blank-project final acceptance are still open.
