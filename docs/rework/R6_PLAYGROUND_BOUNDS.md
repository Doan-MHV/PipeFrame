# R6 Playground render bounds

September 12, 2026. Status: visual bounds clipping verified; painting/crop tools open.

SceneProjectRuntime clips the plain-color tile geometry to an attached Playground's
local rectangle before applying the object's rotation/scale/translation. Selection
already uses that same Playground rectangle. A Tilemap without a Playground component
retains its full geometry. The shared imported tilemap data is never changed by clipping.

ClipTilemapGeometry in Environment/TilemapGeometry.h consumes the six-vertex plain-color
quads emitted by BuildTilemapGeometry. It clips partial edge cells and omits fully outside
cells. It reuses the caller's output vector; source/output must be separate containers.
This is not a general triangle or textured-UV clipping API.

Changing Playground rows/columns/cell size hides or reveals rendered cells. Growing the
bounds restores previously hidden cells. The existing scene property history remains
responsible for undo/redo of those settings. Asset resize/resampling/cropping and their
preview/undo semantics remain unimplemented. Grid resolution is still independent of the
attached asset's cell size; mismatched grids are not automatically resampled.

Verification: TilemapRegression, SceneProjectRuntimeAcceptance and ProjectBuildAcceptance
**3/3 passed**, 9.96 seconds. Tests check partial clipping, fully outside tiles, unchanged
source cells and actual runtime render submissions before shrinking/after expanding bounds.
Dependency lint passed. Evidence: `evidence/r6-playground-bounds/tests.txt`.
No new native screenshot or FPS acceptance was performed. Plugin ABI remains 7.

Remaining: editor brush/palette gestures, bounds-aware painting and physics queries,
asset crop/resize workflows, Ant component integration and the other R6 gates.
