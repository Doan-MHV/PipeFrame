# Shared sprite and kinematic authoring

Implemented for projects derived from `SceneProjectRuntime`. Generated projects
inherit this runtime automatically. This is the default 2D rendering/movement
path; the specialized Ant runtime continues to own its animated, batched ant renderer.

## Create a textured moving object

1. Create/open a generated project and import an image into `Assets/Textures`.
2. In the hierarchy's creation picker, choose **Sprite**. It includes Transform and
   Sprite Renderer. No new C++ entity class is required.
3. Assign the texture to Sprite Renderer / Texture using the asset browser's field
   assignment action. Set Size in world units, Tint and Sorting Order. Higher sprite
   sorting orders draw later. Size is centered on the entity's transform.
4. Use **Add Component / Behaviour** to attach **Environment Collider**. Choose Box,
   set Box Size, and turn Visible off if you do not want its diagnostic shape drawn.
   Keep Collision Enabled on. Sprite size and collider size are independent.
5. Attach **Kinematic Body**, set Velocity and Collision Mask, then press Play.
   An enabled non-trigger Box collider supplies the body's rotated/scaled geometry.
   Without one, the body uses its Fallback Circle Radius in world units.
6. Paint solid terrain in a Playground's map or create Environment Obstacles. The
   shared fixed-step runtime sweeps the moving body and slides it along surfaces.
   Pause stops movement; Reset restores authored component values. Save/reopen and
   build/reload retain the asset reference and component settings.

An unassigned Sprite draws a tinted rectangle, useful during blockout. A nonempty
missing/wrong-type texture reference logs an asset error and does not draw an old
texture. Texture resources are cached by asset ID and refreshed when reimport changes
its revision. Adjacent sprites using the same texture are batched after stable sort.
Sprites render after terrain and collider diagnostics. Sorting currently orders
sprites against other sprites, not arbitrary world layers.

## Extend without rebuilding the basics

- Project behaviour derives from `Behaviour` and is registered with
  `RegisterBehaviour<T>(id, displayName)`. Use its fixed-update lifecycle to change
  body velocity. Declare editable project data with a component-owned `Schema()`.
- Custom renderers derive from `RenderLayer`. A `SceneProjectRuntime` subclass can
  call `AddRenderLayer<MyLayer>(...)`; the runtime owns it and renders it after the
  built-in scene layers. `SetLayerEnabled(false)` disables a layer.
- `GetVisualAssets().ResolveTexture(reference)` provides the same typed asset and
  revision-aware GPU resource path to custom layers. `SpriteRenderLayer` can also be
  embedded in a custom RenderingWorld: Clear, Submit component/transform pairs,
  then Render. Ant-specific leg animation need not be rewritten as separate sprites.
- Custom movement can reuse `SweepBox`, `SweepBoxTilemap`, circle sweep/slide, and
  `EnvironmentQueries`. They use PipeFrame geometry and rendering types, not SFML.
  For full custom movement, disable the built-in Kinematic Body so the object is
  not moved twice. EnvironmentQueries remains available through scene services.

## Explicit physical limits

This is kinematic 2D collision, not a force-driven rigid-body solver. There is no
mass, torque, tire friction, joint or suspension simulation in this addition.
Box translation uses continuous collision against static box/segment obstacles and
transformed/clipped solid tilemaps. Rotation is evaluated at the current pose;
there is no continuous angular sweep. Initial penetration stops motion rather than
teleporting the object out. Moving obstacles and multiple kinematic boxes do not
provide symmetric rigid-body response or box relative-motion CCD. Plan wheel forces
and physical car dynamics separately if the robot project requires them.

Collision and trigger enter/exit notifications use box geometry for box bodies.
Triggers do not block movement. Existing circle-body behavior remains available.

## Verification

`SpritePhysicsAcceptance` covers typed texture loading/reimport, batching/sorting,
geometry/picking, fast and rotated box movement, tile collision and trigger crossing.
`BlankProjectAcceptance` creates a Sprite through ProjectSession's editor actions,
assigns an imported texture, attaches collider/body components, and verifies movement,
asset references, pause/reset, saving/reopening and project build/reload. No project
rendering or collision code is added for that car.
