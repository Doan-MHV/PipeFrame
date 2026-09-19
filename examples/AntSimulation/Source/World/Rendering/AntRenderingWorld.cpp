#include "World/Rendering/AntRenderingWorld.h"
#include "World/AntWorld.h"
#include "World/Rendering/SignalBeaconGeometry.h"
#include <PipeFrame/Components/PlaygroundComponent.h>
#include <PipeFrame/Components/TilemapComponent.h>
#include <PipeFrame/Render/RenderContext.h>
#include <chrono>
#include <algorithm>
namespace ant_simulation {
bool AntRenderingWorld::LoadAssets(const std::filesystem::path &assetRoot,std::string &errorMessage) {
    if (!environment.LoadAssets(assetRoot, errorMessage)) {
        return false;
    }

    if (!ants.LoadAssets(assetRoot, errorMessage)) {
        return false;
    }

    if (!shadows.LoadAssets(assetRoot, errorMessage)) {
        return false;
    }

    return true;
}
void AntRenderingWorld::ApplyOptions(const RenderOptions &renderOptions, AntConfiguration &configuration) {
    environment.SetGridEnabled(renderOptions.showGrid);

    environment.SetMarkersEnabled(renderOptions.showMarkers);

    environment.SetWallShadowEnabled(renderOptions.showShadows);

    shadows.SetEnabled(renderOptions.showShadows);

    debug.SetTargetVisible(renderOptions.showTargets);

    debug.SetPhysicsDebugVisible(renderOptions.showPhysicsDebug);

    configuration.dynamicAntColor = renderOptions.dynamicAntColors;

    /*
     * AntPezza exposes marker intensity as a slider from
     * 1 through 20.
     *
     * A lower power produces brighter/stronger trails.
     */
    const int clampedIntensity = std::clamp(renderOptions.markerIntensity, 1, 20);

    const float intensityRatio = static_cast<float>(clampedIntensity - 1) / 19.0f;

    constexpr float MinimumPower{0.025f};

    constexpr float MaximumPower{0.5f};

    const float markerColorPower = (1.0f - intensityRatio) * (MaximumPower - MinimumPower) + MinimumPower;

    environment.SetMarkerColorPower(markerColorPower);
}
void AntRenderingWorld::DrawFrame(RenderContext &context, AntWorld &world,
    const pipeframe::RegisteredEntityObjects &registeredObjects,
    pipeframe::VisualAssetModule &environmentVisuals, const AntEditorTool &editorTool,
    const AntInspectorData &inspectorData, std::optional<AntId> selectedAnt,
    const RenderOptions &renderOptions, RenderStatistics &renderStatistics,
    const std::function<void()> &drawSelection) {
    const auto viewport = pipeframe::Rectanglef{context.GetCameraCenter()-context.GetCameraSize()*.5f,context.GetCameraSize()};

    const float renderZoom = float(std::max(1,context.GetViewportRectangle().size.x))/std::max(.001f,context.GetCameraSize().x);

    AntEnvironment &simulationEnvironment = world.GetEnvironment();

    const std::span<const ColonyView> colonies = world.GetColonyLifecycleSystem().GetColonies();

    const std::span<const AntView> antViews = world.GetAntQuery().GetAnts();

    const std::span<const AntPhysicsBody> physicsBodies = world.GetPhysicsBodies().GetBodies();

    using Clock = std::chrono::steady_clock;

    using Milliseconds = std::chrono::duration<float, std::milli>;

    const auto geometryStart = Clock::now();

    environment.Update(simulationEnvironment, colonies, viewport);

    if (renderOptions.showAnts) {
        ants.UpdateGeometry(antViews, viewport, renderZoom);

        shadows.Update(ants.GetGeometry(), ants.GetMode());
    }

    debug.SetSelectedAnt(inspectorData.highlight ? selectedAnt : std::nullopt);

    debug.SetTargetVisible(renderOptions.showTargets || inspectorData.showTarget);

    debug.Update(antViews, physicsBodies, viewport);

    editor.Update(editorTool, simulationEnvironment, viewport);

    const auto geometryEnd = Clock::now();

    ShowAnts(renderOptions.showAnts);

    environment.SetGround(std::nullopt);
    environment.SetTerrainVisible(true);
    for (const auto &[id, object] : registeredObjects.All())
        if (const auto *ground = object.GetComponent<pipeframe::PlaygroundComponent>()) {
            if (const auto *tiles = object.GetComponent<pipeframe::TilemapComponent>())
                environment.SetTerrainVisible(tiles->visible);
            auto display = *ground;
            display.showGrid = display.showGrid && renderOptions.showGrid;
            pipeframe::RenderState state;
            pipeframe::Vector2f uv;
            if (const auto *material = environmentVisuals.ResolveMaterial(ground->material);
                material && material->error.empty()) {
                display.color = pipeframe::MultiplyTint(ground->color, material->value.tint);
                state = material->State();
                uv = {material->textureSize.x * material->value.uvScale.x,
                      material->textureSize.y * material->value.uvScale.y};
            }
            environment.SetGround(display, state, uv);
        }
    Render(context.GetCanvas(), 0);
    for (const auto &[id, object] : registeredObjects.All()) {
        const auto *transform = object.GetComponent<pipeframe::Transform2DComponent>();
        const auto *settings = object.GetComponent<SignalBeaconComponent>();
        if (!transform || !settings)
            continue;
        const auto vertices = BuildSignalBeaconGeometry(*transform, *settings);
        context.GetCanvas().Draw(vertices.data(), vertices.size(), pipeframe::PrimitiveTopology::Triangles);
    }

    drawSelection();

    Render(context.GetCanvas(), 1);

    const AntRendererStatistics &statistics = ants.GetStatistics();

    if (renderOptions.showAnts) {
        renderStatistics.candidates = statistics.candidateCount;

        renderStatistics.visible = statistics.visibleCount;

        renderStatistics.vertices = statistics.vertexCount;

        renderStatistics.usingQuads = statistics.mode != AntRenderingMode::Points;
    } else {
        renderStatistics.candidates = 0;
        renderStatistics.visible = 0;
        renderStatistics.vertices = 0;
        renderStatistics.usingQuads = false;
    }

    renderStatistics.geometryTimeMs = Milliseconds(geometryEnd - geometryStart).count();
}
}
