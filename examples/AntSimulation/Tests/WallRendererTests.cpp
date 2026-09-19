#include "Configuration/AntConfiguration.h"
#include "World/Rendering/WallRenderer.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/WallBuilder.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Require(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';

        std::exit(1);
    }
}

bool NearlyEqual(const float first, const float second, const float tolerance = 0.0001f) {
    return std::abs(first - second) <= tolerance;
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

    AntEnvironment environment;
    std::string errorMessage;

    Require(environment.Initialize(configuration, errorMessage), "Environment should initialize.");

    Require(environment.AddWall({8.5f, 8.5f}), "Center wall should be added.");

    Require(environment.AddWall({7.5f, 8.5f}), "Left wall should be added.");

    Require(environment.AddWall({8.5f, 7.5f}), "Top wall should be added.");

    Require(WallBuilder::GetWallType(environment, {8, 8}) == WallType::SouthEast,
            "Center wall should use the southeast corner shape.");

    const pipeframe::Rectanglef viewport{
        {6.0f, 6.0f},
        {5.0f, 5.0f},
    };

    WallRenderer renderer;

    renderer.Rebuild(environment, viewport);

    Require(renderer.GetCandidateCount() == environment.GetWallCount(),
            "Candidate count should include every world wall.");

    Require(renderer.GetVisibleWallCount() == 3, "Only the three interior walls should be visible.");

    Require(renderer.GetWallVertices().size() == 18, "Each visible wall should reserve six vertices.");

    Require(renderer.GetShadowVertices().size() == 18, "Each visible wall should have matching shadow geometry.");

    const pipeframe::Vertex2D &wallVertex = renderer.GetWallVertices()[0];

    const pipeframe::Vertex2D &shadowVertex = renderer.GetShadowVertices()[0];

    Require(NearlyEqual(shadowVertex.position.x, wallVertex.position.x + WallRenderer::ShadowOffset.x),
            "Wall shadow should include X offset.");

    Require(NearlyEqual(shadowVertex.position.y, wallVertex.position.y + WallRenderer::ShadowOffset.y),
            "Wall shadow should include Y offset.");

    Require(wallVertex.color == WallRenderer::WallColor, "Walls should use AntPezza wall color.");

    Require(shadowVertex.color == WallRenderer::ShadowColor, "Wall shadows should use configured shadow color.");

    renderer.SetShadowEnabled(false);

    Require(renderer.GetShadowVertices().empty(), "Disabling shadows should clear shadow geometry.");

    renderer.Rebuild(environment, viewport);

    Require(renderer.GetShadowVertices().empty(), "Disabled shadows should not rebuild.");

    std::cout << "All wall renderer tests passed.\n";

    return 0;
}
