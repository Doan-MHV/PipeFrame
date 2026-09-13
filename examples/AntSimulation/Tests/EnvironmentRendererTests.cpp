#include "ColonyFixture.h"
#include "World/Runtime/ColonyView.h"
#include "Configuration/AntConfiguration.h"
#include "World/Rendering/EnvironmentRenderer.h"
#include "World/Runtime/Environment/AntEnvironment.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Require(
    const bool condition,
    const char *message
) {
    if (!condition) {
        std::cerr
            << "FAILED: "
            << message
            << '\n';

        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace ant_simulation;

    AntConfiguration configuration;

    configuration.worldSize = {
        16,
        16,
    };

    configuration.colonyPosition = {
        8.0f,
        8.0f,
    };

    configuration.colonyRadius =
        2.0f;

    AntEnvironment environment;
    std::string errorMessage;

    Require(
        environment.Initialize(
            configuration,
            errorMessage),
        "Environment should initialize.");

    const WorldEntityId foodId =
        environment.AddFood(
            {7.5f, 7.5f},
            4);

    Require(
        foodId != InvalidWorldEntityId,
        "Food should be added.");

    AntWorldCell *markerCell =
        environment.TryGetCell(
            4,
            4);

    Require(
        markerCell != nullptr,
        "Marker cell should exist.");

    markerCell->AddMarker(
        MarkerKind::ToHome,
        configuration.markerMaxIntensity,
        1);

    ColonyFixture fixture{
            1,
            {8.0f, 8.0f},
            configuration.toFoodAntColor,
            configuration,
        };
    std::array<ColonyView, 1> colonies{fixture};

    EnvironmentRenderer renderer(
        configuration);

    renderer.SetMarkersEnabled(true);

    const pipeframe::Rectanglef viewport{
        {0.0f, 0.0f},
        {16.0f, 16.0f},
    };

    renderer.Update(
        environment,
        colonies,
        viewport);

    Require(
        renderer.GetBackgroundVertices().size() ==
            6,
        "Background should contain one quad.");

    Require(
        renderer.GetGridVertices().size() ==
            68,
        "A 16 by 16 world should produce 34 grid lines.");

    Require(
        renderer.GetVisibleFoodCount() == 1,
        "One food entity should be visible.");

    Require(
        renderer.GetFoodVertices().size() ==
            6,
        "One food entity should use one quad.");

    Require(
        renderer.GetVisibleColonyCount() == 1,
        "One colony should be visible.");

    Require(
        renderer.GetColonyFillVertices().size() ==
            EnvironmentRenderer::
                ColonySegmentCount *
            3,
        "Colony fill should contain one triangle per segment.");

    Require(
        renderer.GetColonyOutlineVertices().size() ==
            EnvironmentRenderer::
                ColonySegmentCount *
            6,
        "Colony outline should contain two triangles per segment.");

    Require(
        renderer.GetColonyShadowVertices().size() ==
            EnvironmentRenderer::
                ColonySegmentCount *
            3,
        "Colony shadow should match colony fill geometry.");

    Require(
        renderer.GetMarkerRenderer()
                .GetVisibleMarkerCount() ==
            1,
        "One marker should be visible.");

    Require(
        renderer.GetWallRenderer()
                .GetVisibleWallCount() >
            0,
        "Environment border walls should be visible.");

    renderer.SetGridEnabled(false);

    renderer.Update(
        environment,
        colonies,
        viewport);

    Require(
        renderer.GetGridVertices().empty(),
        "Disabling the grid should clear grid geometry.");

    const pipeframe::Rectanglef hiddenViewport{
        {100.0f, 100.0f},
        {10.0f, 10.0f},
    };

    renderer.Update(
        environment,
        colonies,
        hiddenViewport);

    Require(
        renderer.GetVisibleFoodCount() == 0,
        "Off-screen food should be culled.");

    Require(
        renderer.GetVisibleColonyCount() == 0,
        "Off-screen colonies should be culled.");

    std::cout
        << "All environment renderer tests passed.\n";

    return 0;
}
