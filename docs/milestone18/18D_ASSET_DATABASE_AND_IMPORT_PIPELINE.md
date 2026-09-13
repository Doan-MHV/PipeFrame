# Milestone 18D — Asset database, browser, and import pipeline

Status: complete.

## Persistent asset identity

PipeFrame now owns a project asset database at `.pipeframe/assets.db`. Every
asset receives a stable ID independent of its filename or folder. Records retain
the authored source path, imported cache output, thumbnail and preview paths,
type, state, importer ID/version, revision, deterministic content hash,
dependencies, tags, and the most recent error. Moving or reimporting an asset
keeps the ID, so scene and component references do not change.

The format is versioned and loads the previous version-1 record layout. Writes
use a temporary file and replace the database after a complete write. Database
validation reports duplicate IDs, unsafe paths, missing dependencies, malformed
part attachments, and invalid configuration ranges.

## Import pipeline

The built-in import registry covers:

- textures: PNG, JPEG, BMP, and TGA;
- audio: WAV, OGG, and FLAC;
- shaders: GLSL, vertex, and fragment sources;
- materials: MAT and PFMAT;
- models: OBJ, glTF, and GLB;
- PipeFrame scenes; and
- generic PipeFrame part assets.

External sources are copied into the project's standard `Assets` folder,
validated by format, and packaged into a deterministic versioned
`compiled.pfasset` artifact under `.pipeframe/cache/<asset-id>`. Importers also
produce separate deterministic preview and thumbnail artifacts plus discovered
dependency metadata. FNV-1a content hashes make unchanged and changed inputs
reproducible without timestamps. The registry API accepts additional importer
callbacks without changing the database.

Imports and reimports are represented by queued operations with Queued,
Running, Succeeded, Failed, and Cancelled states. They can be inspected and
cancelled before processing or cooperatively while an importer runs. Import or
reimport errors remain visible in the operation history and Asset Browser.
Missing source files enter the Missing state. Repairing one copies the
replacement into the project, retains the asset ID, and reimports it.

## Generic part assets

Part metadata is brand-independent. It records category, physical size,
collision default, typed attachment definitions and local transforms,
compatibility tags, preview data, ordinary asset tags, and named configuration
limits with units. It contains no ELEGOO, Arduino, ultrasonic, LiDAR, wheel, or
other product-specific policy. Milestone 19 can define those assets with this
schema.

## Asset Browser and scene references

The Workbench toolbar opens an Asset Browser in the existing tool column. It
replaces the Inspector while open, preserving the viewport and preventing panel
overlap. The browser provides search, type filtering, state/type rows, import,
selection, assignment, reimport, cancellation, and visible failure status.

Assignment uses a stable `AssetReference` and passes through `ProjectSession`'s
typed, undoable property transaction. Missing references can be enumerated and
repaired across object and component properties in one undo step. The same
assignment command handles the explicit Assign action and dragging an asset row
onto the viewport; it rejects unknown or non-ready asset IDs.

New projects create standard Audio, Materials, Models, Parts, Prefabs, Shaders,
and Textures source folders plus the private imported cache folder.

## Acceptance evidence

- `AssetDatabaseRegression` imports every built-in category, validates search
  and dependencies, round-trips generic part metadata, preserves identity
  through move/reimport/repair, verifies deterministic revision/hash changes,
  checks queued and running cancellation, rejects malformed format content,
  keeps cache/preview/thumbnail artifacts distinct, exposes failures, and
  migrates a version-1 database.
- `WorkbenchUIAcceptance` verifies the non-overlapping Asset Browser workflow,
  imported-asset visibility, selection and assignment, rejected unknown IDs,
  viewport drag/drop, missing-reference discovery, transactional repair, and
  Inspector recovery.
- `ProjectManagerRegression` verifies the standard source/cache folders in a
  newly created project.

The complete Debug build succeeds and CTest passes 70/70 tests.
