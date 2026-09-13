# PipeFrame resource, render, and audio services

Milestone 17E moves reusable resource lifetime and render-service behavior into
PipeFrame while each simulation continues to decide what it wants to draw.
SFML remains the first private backend implementation.

## Ownership

| Concern | Public PipeFrame contract | Private SFML implementation |
|---|---|---|
| Textures, fonts, shaders | `GraphicsResourceService` and typed handles | `GraphicsResourceService.cpp` |
| Offscreen render surfaces | `RenderSurfaceHandle` | `sf::RenderTexture` instances owned by the graphics service |
| Decoded images | `ImageData`, `LoadImageData`, `SaveImageData` | SFML image codec in `ImageData.cpp` |
| Audio clips and voices | `AudioService`, `AudioClipHandle` | sound buffers and active sounds in `AudioService.cpp` |
| Geometry submission | `GeometryCommand`, `VertexBatch2D`, `Path2D` | `GeometryRenderer.cpp` |
| Effects | `EffectGraph`, `EffectPass`, `PingPongSurface` | backend resolves handles and executes supported passes |
| Utility drawing | `ParticleSystem2D`, `DebugDraw2D` | geometry-command submission |

Handles do not expose backend objects. Repeated loads of the same resource key
return the cached handle, and services own the resource until they are cleared
or destroyed. `ResourceState` reports unloaded, ready, lost, and failed states;
the generic `AssetRegistry` retains an error string and revision for loss and
reload diagnostics.

## Application boundary

Ant renderers now store texture, font, and shader handles. Its world-map loader
uses `ImageData`, so image decoding is no longer part of Ant domain code.
SailBoat stores texture, font, shader, and render-surface handles, routes the
mark sound through `AudioService`, and submits its preview trajectory through a
PipeFrame path command.

Applications may still contain the small SFML draw adapters recorded by the
Milestone 17 allowlist. They may borrow a resolved backend pointer for the
duration of a draw call, but they cannot own SFML textures, shaders, render
targets, sounds, vertex arrays, fonts, images, or windows. Dependency lint
enforces this rule on both example source trees.

## Failure behavior

Failed loads return a valid typed handle whose state is `Failed`, allowing UI
and render code to continue with a fallback. Unsupported or unavailable blur,
shadow, and custom shader passes resolve to a copy pass. SailBoat falls back to
its non-shader water rendering, and Ant shadows can render without their shader.
Resource regressions cover cache hits, loss, reload, error reporting, shader
fallback resolution, ping-pong surface swaps, paths, particles, debug drawing,
and audio load failure.
