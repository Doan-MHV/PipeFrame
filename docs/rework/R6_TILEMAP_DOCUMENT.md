# R6 editable tilemap asset document

2026-09-12 — engine document integration implemented; Workbench painting remains open.

`engine/include/PipeFrame/Editor/TilemapAssetEditor.h` implements the existing
EditorTool contract and owns both a mutable Tilemap2D and its TilemapPaintTool.
The renderer's cached resource is never cast to mutable data. Map and gesture
lifetimes are ordered explicitly. The host retains ownership of AssetDatabase.

A completed gesture records one already-applied patch. Undo/redo cancels any active
preview and applies the patch atomically. History tracks the saved cursor, including
branching after undo. No-op/cancelled strokes create no history. Document access is
const so callers cannot silently bypass history. Layer/definition editing is not
provided by this document API yet.

Open rejects replacing dirty documents or active gestures. Close requires explicit
discard for unsaved work. Save rejects active gestures, a changed project, moved asset,
or source contents changed externally. It serializes to a same-directory temporary
file, closes it, renames it onto the source, then reimports the same asset ID through
the existing database. Cache revision updates are consumed by TilemapAssetModule.
Existing temporary files block saving rather than being overwritten.

If reimport fails after source replacement, the source remains saved but the editor
stays dirty and reports the error so reimport can be retried. This is not a transaction
across source, cache and database. Save is synchronous, detects external changes at
save time, and is not a filesystem lock or a crash-durability/fsync guarantee.

## Host usage

1. Open a Tilemap asset ID with `Open(database, id)`.
2. Configure the selected layer/brush/shape and enable the tool.
3. Route mapped pointer events through `HandleEvent`; draw `Document()` and `Preview()`.
4. Route asset Undo/Redo/Save to this object while asset editing has focus.
5. Close/save/discard explicitly before project switch and plugin unload.

The document edits the shared asset. A future Make Unique action must clone/import
a new asset and update the selected entity reference before opening that asset.
There is no implicit cloning or per-entity override.

## Verification and remaining work

AssetDatabaseRegression covers open, gestures, undo/redo, saved-cursor branching,
source reload, stable ID/revision publication, clean-save no-op, cancellation,
external edits, explicit discard and temporary-file failure/retry. TilemapRegression
and dependency lint are also run. Evidence: [tests](evidence/r6-tilemap-document/tests.txt).

Workbench does not instantiate this class yet. Palette/preview UI, camera and
Playground mapping, focused asset history routing, save/discard prompts, asset
creation, shared/make-unique choice, Ant integration and native interaction
acceptance remain open. This slice does not complete R6 or provide a visible paint tool.
