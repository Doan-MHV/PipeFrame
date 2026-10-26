#pragma once
#include <PipeFrame/Render/GeometryDraw.h>
#include <PipeFrame/Render/RenderSystem.h>
namespace pipeframe {
// Compatibility names for existing project physics and mesh collectors.
using WorldDebugOptions = RenderOptions;
using WorldDebugDraw = GeometryDraw;
} // namespace pipeframe
