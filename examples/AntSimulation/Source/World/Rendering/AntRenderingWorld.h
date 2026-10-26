#pragma once
#include "Editor/AntEditorToolRenderer.h"
#include "World/Rendering/AntDebugRenderer.h"
#include "World/Rendering/AntRenderer.h"
#include "World/Rendering/EnvironmentRenderer.h"
#include "World/Rendering/ShadowRenderer.h"
#include <PipeFrame/World/World.h>
#include <PipeFrame/Project/EntityRegistry.h>
#include <PipeFrame/Environment/VisualAssetModule.h>
#include "Editor/AntInspector.h"
class RenderContext;
#include <PipeFrame/Render/WorldDebugView.h>
namespace ant_simulation {
class AntWorld;
class AntRenderingWorld final : public pipeframe::RenderingWorld {
  public:
    struct RenderStatistics {
        std::size_t candidates{0};
        std::size_t visible{0};
        std::size_t vertices{0};

        float movementTimeMs{0.0f};
        float spatialGridTimeMs{0.0f};
        float geometryTimeMs{0.0f};

        bool usingQuads{false};
    };

    struct RenderOptions {
        bool showGrid{true};
        bool showMarkers{true};
        bool showShadows{true};
        bool showTargets{false};
        bool showPhysicsDebug{false};

        bool showAnts{true};
        bool dynamicAntColors{false};

        int markerIntensity{10};
    };

    bool LoadAssets(const std::filesystem::path &, std::string &);
    void ApplyOptions(const RenderOptions &, AntConfiguration &);
    void DrawFrame(RenderContext &, AntWorld &, const pipeframe::RegisteredEntityObjects &,
                   pipeframe::VisualAssetModule &, const AntEditorTool &, const AntInspectorData &,
                   std::optional<AntId>, const RenderOptions &, RenderStatistics &,
                   const std::function<void()> &drawSelection);
    explicit AntRenderingWorld(const AntConfiguration &configuration)
        : environment(configuration), ants(configuration), editor(configuration) {
        AddLayer(environment, 0);
        AddLayer(shadows, 1);
        AddLayer(ants, 1);
        AddLayer(debug, 1);
        AddLayer(editor, 1);
    }
    void ShowAnts(bool enabled) {
        ants.SetLayerEnabled(enabled);
        shadows.SetLayerEnabled(enabled);
    }
    void CollectDebug(pipeframe::WorldDebugDraw &draw) const {
        if(ants.GetMode()==AntRenderingMode::Points)return; // Point LOD has no triangle mesh.
        const auto &geometry=ants.GetGeometry();
        draw.Mesh(geometry.GetBodyVertices());
        if(ants.GetMode()==AntRenderingMode::DetailedQuads)draw.Mesh(geometry.GetLegVertices());
        draw.Mesh(geometry.GetFoodVertices());
    }
    EnvironmentRenderer environment;
    AntRenderer ants;
    ShadowRenderer shadows;
    AntDebugRenderer debug;
    AntEditorToolRenderer editor;
};
} // namespace ant_simulation
