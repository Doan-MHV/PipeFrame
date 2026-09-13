#include <PipeFrame/Backend/SFML/CanvasAdapter.h>
#include "Entities/ColonyEntity.h"
#include "World/Runtime/AntQuery.h"
#include "World/Runtime/ColonyView.h"
#include "Configuration/AntConfiguration.h"
#include "World/Rendering/AntRenderer.h"
#include "World/Rendering/EnvironmentRenderer.h"
#include "World/Rendering/ShadowRenderer.h"
#include "World/Runtime/Environment/AntEnvironment.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/View.hpp>

#ifndef ANT_SIMULATION_ASSET_ROOT
#error "ANT_SIMULATION_ASSET_ROOT must identify the AntSimulation Assets directory."
#endif

namespace {

using Clock = std::chrono::steady_clock;

void Require(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

double RenderFrames(
    sf::RenderTexture &target,
    ant_simulation::EnvironmentRenderer &environmentRenderer,
    ant_simulation::AntRenderer &antRenderer,
    ant_simulation::ShadowRenderer &shadowRenderer,
    const ant_simulation::AntEnvironment &environment,
    const std::span<const ant_simulation::ColonyView> colonies,
    const std::span<const ant_simulation::AntView> ants,
    const pipeframe::Rectanglef viewport,
    const float zoom,
    const std::size_t frameCount
) {
    const auto started = Clock::now();

    for (std::size_t frame = 0; frame < frameCount; ++frame) {
        environmentRenderer.Update(environment, colonies, viewport);
        antRenderer.UpdateGeometry(ants, viewport, zoom);
        shadowRenderer.Update(antRenderer.GetGeometry(), antRenderer.GetMode());

        target.clear(sf::Color(22, 22, 22));
        environmentRenderer.Draw(pipeframe::backend::sfml::MakeCanvas(target));
        shadowRenderer.Draw(pipeframe::backend::sfml::MakeCanvas(target));
        antRenderer.Draw(pipeframe::backend::sfml::MakeCanvas(target));
        target.display();
    }

    return std::chrono::duration<double, std::milli>(Clock::now() - started).count() /
           static_cast<double>(frameCount);
}

} // namespace

int main() {
    using namespace ant_simulation;

    constexpr std::size_t TwoColonyAntCount{2'000};
    constexpr std::size_t LodAntCount{10'000};
    constexpr std::size_t FrameCount{60};

    AntConfiguration configuration;
    configuration.worldSize = {384, 216};

    AntEnvironment environment;
    std::string errorMessage;
    Require(environment.Initialize(configuration, errorMessage), "Render benchmark environment should initialize.");

    for (int y = 3; y < configuration.worldSize.y - 3; y += 3) {
        for (int x = 3; x < configuration.worldSize.x - 3; x += 3) {
            AntWorldCell *cell = environment.TryGetCell(x, y);
            if (cell != nullptr && !cell->wall) {
                cell->AddMarker(
                    (x + y) % 2 == 0 ? MarkerKind::ToHome : MarkerKind::ToFood,
                    configuration.markerMaxIntensity * 0.35f,
                    (x < configuration.worldSize.x / 2) ? 1 : 2);
            }
        }
    }

    pipeframe::BehaviourScene colonyScene;
    std::vector<ColonyView> colonies;
    colonies.emplace_back(colonyScene.Instantiate(ColonyEntity(1, pipeframe::Vector2f{96.0f, 108.0f}, pipeframe::Color(239, 71, 111), configuration)));
    colonies.emplace_back(colonyScene.Instantiate(ColonyEntity(2, pipeframe::Vector2f{288.0f, 108.0f}, pipeframe::Color(6, 214, 160), configuration)));

    pipeframe::BehaviourScene antStoreScene;
    AntQuery antStore(antStoreScene);
    for (std::size_t index = 0; index < LodAntCount; ++index) {
        const ColonyId colonyId = index % 2 == 0 ? 1 : 2;
        AntView ant = antStore.Create(
            colonyId,
            AntRole::Follower,
            {
                2.5f + static_cast<float>(index % 379),
                2.5f + static_cast<float>((index / 379) % 211),
            },
            static_cast<float>(index % 360) * AntConfiguration::DegreesToRadians(1.0f),
            0.0f,
            configuration);
        ant.Identity().color = colonyId == 1 ? pipeframe::Color(239, 71, 111) : pipeframe::Color(6, 214, 160);
    }

    const std::filesystem::path assetRoot{ANT_SIMULATION_ASSET_ROOT};

    EnvironmentRenderer environmentRenderer(configuration);
    AntRenderer antRenderer(configuration);
    ShadowRenderer shadowRenderer;

    Require(environmentRenderer.LoadAssets(assetRoot, errorMessage), "Environment assets should load.");
    Require(antRenderer.LoadAssets(assetRoot, errorMessage), "Ant assets should load.");
    Require(shadowRenderer.LoadAssets(assetRoot, errorMessage), "Shadow assets and shader should load.");

    sf::RenderTexture target({1920, 1080});
    sf::View view;
    view.setCenter({192.0f, 108.0f});
    view.setSize({384.0f, 216.0f});
    target.setView(view);

    const pipeframe::Rectanglef viewport{{0.0f, 0.0f}, {384.0f, 216.0f}};
    const std::span<const AntView> allAnts = antStore.GetAnts();
    const std::span<const AntView> twoColonyAnts{allAnts.data(), TwoColonyAntCount};

    const double twoColonyFrameMs = RenderFrames(
        target,
        environmentRenderer,
        antRenderer,
        shadowRenderer,
        environment,
        colonies,
        twoColonyAnts,
        viewport,
        5.0f,
        FrameCount);

    Require(antRenderer.GetMode() == AntRenderingMode::SimpleQuads,
            "2K render scenario should use automatic simple-quad LOD.");
    Require(antRenderer.GetStatistics().visibleCount == TwoColonyAntCount,
            "2K render scenario should draw both complete colonies.");

    const double lodFrameMs = RenderFrames(
        target,
        environmentRenderer,
        antRenderer,
        shadowRenderer,
        environment,
        colonies,
        allAnts,
        viewport,
        1.0f,
        FrameCount);

    Require(antRenderer.GetMode() == AntRenderingMode::Points,
            "10K render scenario should automatically select point LOD.");
    Require(antRenderer.GetStatistics().visibleCount == LodAntCount,
            "10K point-LOD scenario should retain every visible ant.");
    Require(antRenderer.GetStatistics().vertexCount == LodAntCount,
            "10K point LOD should use exactly one vertex per ant.");

    std::cout << "2K two-colony quads + shadows + markers: " << twoColonyFrameMs
              << " ms/frame\n"
              << "10K automatic point LOD + markers: " << lodFrameMs
              << " ms/frame\n"
              << "All ant render performance tests passed.\n";
    return 0;
}
