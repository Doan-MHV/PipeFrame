# R6 package 2 — Playground, tileset, tilemap and material assets

September 12, 2026. Implements the reusable 2D asset package. R6 as a whole
remains open; custom Ant brushes, broader environment integration, performance
acceptance and the final learning workflow are separate packages.

## Authoring workflow

1. Create a project and add **Playground** through the hierarchy picker. Its
   registered components expose transform, ground dimensions, grid, material and
   tilemap. Solid-color ground works without any image or project-side code.
2. Open **Assets → Textures → Import**. Texture decoding runs on a worker. The
   browser shows importing/queued/failed counts and supports Cancel. Reimport
   uses the same queue. GPU resources are created on the rendering thread.
3. In **Materials**, choose **New Asset**. Pick a ready texture, set Tint,
   Ground UV Repeats and Smooth Filtering, then **Save Asset**. A material with
   no texture is a solid tint. Field validation appears in the asset editor.
   Select the material row to preview it. With a Playground selected, use the
   explicit **Assign to Playground / Material** action. These targets come from
   the selected object's component schemas, not project-specific code.
4. In **Tilesets**, choose **New Asset**, pick a textured material and enter
   tile width/height in pixels, columns/rows, outer margin and spacing. IDs start
   at 1 and run left-to-right, top-to-bottom. Zero is empty. Save validates that
   the complete atlas fits the texture. Its preview outlines tile regions.
5. Create/edit a map in **Maps**, select its tileset, choose a layer and tile ID,
   and paint in the viewport. Tile definition tint multiplies material tint;
   collision flags remain independent of the texture. Painting previews use the
   same atlas coordinates as committed cells. Existing plain-color maps remain valid.
6. Save the map and scene. Stable ID links form `Playground → Material → Texture`
   and `Playground → Tilemap → Tileset → Material → Texture`. Reimporting a texture
   refreshes dependent render resources without changing any IDs or scene bindings.

The browser and visual asset inspector use public `ViewPanel`, declarative `View`
and `SchemaInspector`, like simulation UI. Numeric, color, reference and cross-field
validation lives with the asset definitions/editor model. No SFML types enter Ant,
public UI/editor headers, or the material/tilemap authoring API.

## Resize and sharing

**Preview Resize** changes the map's extent and/or cell size while preserving cell
indices in the overlap. The editor reports how many painted cells will be cropped;
its yellow viewport outline previews the resulting bounds. **Apply Size** is one
undo transaction. Undo restores cropped cells, layer flags, tile definitions and
asset references.

**Resample Same Area** changes resolution using nearest cell centers and derives
cell size from the old physical width. Width and height must agree on square-cell
size; incompatible aspect ratios are rejected. This never silently stretches cells.

Playground bounds and map extent are independent. Shrinking Playground clips display,
painting and collision queries without destroying the underlying map. **Make Unique**
copies the saved map and rebinds only the selected Playground. Scene undo restores
the shared binding after Stop Paint.

## Developer API and persistence

- `Material2D::Schema()` and `Tileset2D::Schema()` own exposed fields and validation.
- `VisualAssetEditor` owns draft values, undo/redo, savepoints, linked resource
  checks and external-file conflict protection.
- `TilemapAssetEditor` owns painting and map resize/resample history.
- `VisualAssetModule` resolves stable IDs and owns cached material/texture resources.
  `SceneProjectRuntime` uses it automatically for Playground and Tilemap components.
- Material and tileset formats are version 1 (`.pfmat`, `.pftileset`). Tilemap
  format is version 2 with a tileset ID; version 1 plain-color maps still load.
- Texture importer version 4 retains a full decoded `image.png`, plus the bounded
  preview and thumbnail. Existing texture caches require **Reimport** to obtain
  the full decoded artifact. Database IDs remain stable.
- Plugin ABI is **8** because the host asset service layout changed. Rebuild project
  plugins; old binaries are rejected by the existing ABI check.

All `AssetDatabase` calls belong to its owning thread. `QueueImport`/`QueueReimport`
plus `PumpOperations` run CPU importer callbacks asynchronously. Callbacks receive
copied request data, staging paths and a cancellation probe; they must not capture
and mutate the live database or use GPU APIs. Completed records publish only from
`PumpOperations`. Failed/cancelled reimports retain the previous published cache and
revision. Database destruction cancels and joins the worker. Project switching is
blocked until an active worker finishes/cancels. Synchronous APIs remain available
for build tools and small authoring-document saves; initial asset discovery is still
synchronous when opening a project.

Viewport drops assign only when there is exactly one compatible destination. Multiple
possible destinations require an explicit browser property button. Prefab placement
has its own labeled action.

## Verification

- Full Debug build and **72/72 tests passed** (91.58 seconds), Ant-only configuration.
- Full Release build passed; focused visual-asset and dependency checks **2/2 passed**.
- Native material and tileset editor runs passed. Normal 1440×900 and narrow 800×700
  captures, including scrolled fields/save controls/previews, were inspected.
- Public UI/editor and Ant boundary compilation remains clean. No new backend
  dependency is exposed through these asset APIs.

`VisualAssetAcceptance` covers real two-color texture import, material validation and
history, typed atlas dependencies and dimensions, map painting, crop preview/undo,
resolution resampling, Make Unique, asset reopen, native rendered ground UV repeats
and atlas selection, successful texture reimport and failed-import cache retention,
and cancellation while an importer is deliberately blocked.

The Workbench regression checks explicit asset-to-property routing alongside existing
map editing, shared bindings, input and reload tests. Generated-project build acceptance
retains the blank Playground/create/paint/save/reopen workflow without Ant registration.
Final build/test output and inspected native screenshots are stored under
[evidence/r6-visual-assets](evidence/r6-visual-assets).

This package provides regular rectangular 2D atlases and single-texture materials.
Shader graphs, arbitrary packed/rotated atlas polygons, 3D terrain and asynchronous
initial project discovery are not implemented by this package.
