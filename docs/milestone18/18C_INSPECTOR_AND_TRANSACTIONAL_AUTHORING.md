# Milestone 18C — Inspector and transactional authoring

Status: complete.

## Delivered editor behavior

`InspectorPanel` now builds component sections and controls from
`SceneComponentTypeDescriptor` and `PropertyDescriptor`. It contains no
project-name or project-property branches. The same path renders PipeFrame's
Transform, Physics Body, Collider, Renderer, and Identity components and any
component registered by a project runtime.

The generated controls cover Boolean, Integer, Number, String, Vector2, Color,
Enum, AssetReference, ObjectReference, bounded range metadata, and read-only
telemetry. Labels include declared units and ranges. Component sections are
collapsible. Unsupported or temporarily unavailable plugin component schemas
remain inspectable through a type-inferred fallback instead of losing their
serialized data. `editorHint` is the stable custom-drawer key. A Drawer
extension can register an executable, backend-neutral Inspector presentation
callback for formatted values, detail text, normalized preview values, and an
accent color. The generic kind-based control remains the editable fallback and
callback failures are isolated, so adding a hint or project component never
requires a Workbench property branch.

Multiple selected objects expose mixed values and accept one batch mutation.
Vector and color channel edits preserve the untouched channels on every object,
rather than copying the primary object's full value across the selection.
Component copy/paste and reset-to-default use typed scene data and schema
defaults. These operations, batch edits, and continuous viewport edits create
one undo record and support cancel, undo, and redo.

## Validation and runtime authority

`ProjectSession` is the mutation boundary. It rejects the wrong variant type,
out-of-range numbers, invalid enum values, edits to read-only telemetry, and
dangling object references before recording history. Unknown schemas remain
round-trippable for plugin reload recovery.

`ProjectRuntimeComponentEdit` provides the runtime-safe edit path. A runtime can
consume validated component commands with an explicit Stopped, Paused, or
Playing authoring state. Physics-backed runtimes use this hook for teleports or
body updates and return success; runtimes that do not implement it receive the
existing full-scene synchronization fallback. Undo and redo synchronize the
complete authored scene, restoring the matching runtime state.

Transient runtime values travel in the opposite direction through
`GetLiveComponentProperties`. The Inspector displays those values for
read-only/telemetry fields without writing them into the scene, dirty state, or
undo history. Authored Transform values therefore remain the stopped-state
source of truth while live physics poses and diagnostics remain transient.

## Property exposure API

The typed `ComponentBuilder`/`FieldDescriptor` API remains canonical. Optional
`PF_COMPONENT` and `PF_PROPERTY` macros provide Unreal-style declaration
spelling while retaining explicit stable serialization IDs and typed values.
They do not expose raw offsets, depend on compiler reflection, or let the editor
mutate arbitrary C++ storage. Member-binding code generation belongs to 18H's
source-generation workflow rather than the declaration macros.

Ant and SailBoat now register domain component schemas for every authored
object type. New objects store their settings in those components.
`ProjectSession` migrates legacy flat properties into the matching registered
components when an older reference scene opens, preserving values and removing
duplicate Inspector fields. Runtime readers retain a legacy fallback for old
standalone scene and regression fixtures.

## Acceptance evidence

- `AuthoringFoundationRegression` proves builder and macro registration with
  stable component and field IDs.
- `GenericSceneSerialization` round-trips every Inspector storage kind,
  component schema versions, nested transforms, units, and stable references.
- `WorkbenchUIAcceptance` proves metadata-generated shared/project sections,
  all standard property kinds, foldout interaction, read-only live telemetry,
  mixed values, range/enum/read-only validation, per-channel batch edits,
  copy/paste, reset, one-step undo, an executable drawer supplied by an external
  runtime plugin, first-party scene migration, and supported responsive layouts.
- `ComponentSceneModelAcceptance` covers component/hierarchy transactions,
  required Transform behavior, connections, scene validation, templates, and
  additive scenes.

The complete Debug build succeeds and CTest passes 70/70 tests.
