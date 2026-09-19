#include "Configuration/AntConfiguration.h"
#include "World/Rendering/MarkerRenderer.h"
#include "World/Runtime/Environment/AntEnvironment.h"

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

    AntWorldCell *homeCell = environment.TryGetCell(8, 8);

    Require(homeCell != nullptr, "Home marker cell should exist.");

    homeCell->AddMarker(MarkerKind::ToHome, configuration.markerMaxIntensity, 1);

    MarkerRenderer renderer(configuration);

    renderer.SetEnabled(true);

    const pipeframe::Rectanglef viewport{
        {0.0f, 0.0f},
        {16.0f, 16.0f},
    };

    renderer.ForceUpdate(environment, viewport);

    Require(renderer.GetCandidateCount() == environment.GetCellCount(),
            "Renderer should report every world cell as a candidate.");

    Require(renderer.GetVisibleMarkerCount() == 1, "One active home marker should be visible.");

    Require(renderer.GetVertices().size() == 6, "One marker should use one triangle quad.");

    Require(renderer.GetVertices()[0].color.a > 0, "Visible marker should have non-zero alpha.");

    const WorldEntityId foodId = environment.AddFood({8.5f, 8.5f}, 1);

    Require(foodId != InvalidWorldEntityId, "Food should be added to marker cell.");

    renderer.ForceUpdate(environment, viewport);

    Require(renderer.GetVisibleMarkerCount() == 0, "Markers underneath food should be hidden.");

    Require(environment.RemoveFood({8.5f, 8.5f}), "Food should be removable.");

    homeCell->GetMarker(MarkerKind::ToHome).Clear();

    homeCell->AddMarker(MarkerKind::ToEnemy, configuration.markerMaxIntensity, 1);

    renderer.ForceUpdate(environment, viewport);

    Require(renderer.GetVisibleMarkerCount() == 0, "Enemy-only markers should remain hidden like AntPezza.");

    homeCell->AddMarker(MarkerKind::ToFood, configuration.markerMaxIntensity, 1);

    renderer.ForceUpdate(environment, viewport);

    Require(renderer.GetVisibleMarkerCount() == 1, "A food marker should be visible.");

    renderer.SetEnabled(false);

    Require(renderer.GetVertices().empty(), "Disabling markers should clear their geometry.");

    std::cout << "All marker renderer tests passed.\n";

    return 0;
}
