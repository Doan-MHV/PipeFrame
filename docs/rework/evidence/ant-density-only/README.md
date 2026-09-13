# Ant density-only environment cleanup — 2026-09-13

Removed the legacy visual Food layer and unused yellow food tile definition from
Assets/Tilemaps/Main.pftilemap. Removed Assets/Maps/Main.png and its indexed asset
record/cache. Reimported the tilemap cache without changing its asset ID.

A cell-by-cell comparison against the pre-change saved map verified identical
terrain collision and effective food quantities: 20,062 solid authored cells,
1,896 food cells, 13,497 food units. These include the saved edits present during
this cleanup. Existing density values (including zero) took precedence; no erased
food was resurrected from the old visual layer. Runtime still excludes the protected
border when applying authored cells.

Removed Ant PNG conversion and wall-mask APIs, the legacy map filename editor
property/fallback, and the implicit food-from-image configuration value. New brush
layers initialize to zero using the engine default. Source PNG sprites/shadows and
Pezzza reference material are unrelated and retained.

Release: all 79 enabled Ant-only tests passed. Debug: all four focused tests
passed (WorkbenchUIAcceptance, AntEnvironmentAuthoringAcceptance,
AntSimulationRegression, AntWorldMapLoaderRegression). World-map regression now verifies
numeric quantities, wall priority, paint/erase, serialized round trips, PNG rejection,
invalid input preservation, and the shipped map's density-only structure. Runtime
fixtures and benchmark placement now use tilemaps, not PNGs. Performance tests in
the normal suite passed; no new multi-colony graphical percentile benchmark was run.

Pre-cleanup local backup: /tmp/pipeframe-ant-before-density-only (outside Assets).
An editor already open before this change can still hold an old unsaved map in
memory. Reopen the migrated map rather than saving that stale copy over the disk map.
