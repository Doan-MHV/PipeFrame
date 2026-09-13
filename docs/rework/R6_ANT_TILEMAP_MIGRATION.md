# Ant environments: tilemaps and food density only

Updated 2026-09-13. This supersedes the temporary PNG migration workflow.

- Terrain is authored with solid tiles on collision-enabled tile layers.
- Starting food exists only in the `ant.food-density` numeric layer.
- Selecting **Ant / Ant Food Density** creates missing density data with zeros.
- Paint writes **Food per cell**; erase writes zero. Save Map persists the result.
- The old visual `Food` layer, its yellow tile definition, `Assets/Maps/Main.png`,
  PNG importer, PNG wall masks, and Legacy Map File property have been removed.
- The shipped map was converted once, preserving existing density values where
  present, including zeros. Terrain geometry was retained. The old implicit seven
  food units were written as explicit data only when density was absent.
- Live **FOOD / TOOLS** edits still affect the running simulation until Reset.

Use a Playground with a typed Tilemap asset. Simulation Settings also accepts a
 typed Environment Map asset when a Playground is absent; no image path fallback
 exists. Ant sprite/marker/shadow PNG textures remain rendering assets and are
 unrelated to environment authoring. Original Pezzza reference sources and historical
 evidence are not modified.

The map authoring view displays density as the brush's green data overlay. Ant's
runtime renders food from the same quantities after the map is applied. There is no
separate yellow Food tile layer to erase.
