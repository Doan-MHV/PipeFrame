#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/Marker.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
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
        8,
        6,
    };

    configuration.colonyPosition = {
        3.0f,
        3.0f,
    };

    AntEnvironment environment;
    std::string errorMessage;

    Require(environment.Initialize(configuration, errorMessage), "A valid environment should initialize.");

    Require(errorMessage.empty(), "Successful initialization should not report an error.");

    Require(environment.IsInitialized(), "Environment should report that it is initialized.");

    Require(environment.GetWidth() == 8, "Environment width should match its configuration.");

    Require(environment.GetHeight() == 6, "Environment height should match its configuration.");

    Require(environment.GetCellCount() == 48, "Environment should allocate width multiplied by height cells.");

    for (int y = 0; y < environment.GetHeight(); ++y) {
        for (int x = 0; x < environment.GetWidth(); ++x) {
            const AntWorldCell *cell = environment.TryGetCell(x, y);

            Require(cell != nullptr, "Every in-range cell should be accessible.");

            const bool expectedBorder = x < AntEnvironment::BorderMargin || y < AntEnvironment::BorderMargin ||
                                        x >= environment.GetWidth() - AntEnvironment::BorderMargin ||
                                        y >= environment.GetHeight() - AntEnvironment::BorderMargin;

            Require(cell->wall == expectedBorder, "Environment should create a two-cell wall border.");
        }
    }

    Require(environment.TryGetCell(-1, 0) == nullptr, "Negative cell coordinates should be rejected.");

    Require(environment.TryGetCell(8, 0) == nullptr, "Cell coordinates beyond the width should be rejected.");

    Require(environment.TryGetCell(0, 6) == nullptr, "Cell coordinates beyond the height should be rejected.");

    Require(environment.IsSimulationPositionValid({2.0f, 2.0f}),
            "The first position inside the physics margin should be valid.");

    Require(environment.IsSimulationPositionValid({5.999f, 3.999f}),
            "A position immediately before the far margin should be valid.");

    Require(!environment.IsSimulationPositionValid({1.999f, 2.0f}),
            "A position inside the left border should be invalid.");

    Require(!environment.IsSimulationPositionValid({6.0f, 2.0f}),
            "A position at the right simulation bound should be invalid.");

    Require(!environment.IsSimulationPositionValid({
                std::numeric_limits<float>::quiet_NaN(),
                2.0f,
            }),
            "A non-finite position should be invalid.");

    Require(AntEnvironment::WorldToCell({2.75f, 3.25f}) == pipeframe::Vector2i{2, 3},
            "World positions should be floored to cell coordinates.");

    Require(AntEnvironment::GetCellCenter({2, 3}) == pipeframe::Vector2f{2.5f, 3.5f},
            "Cell centers should have a half-cell offset.");

    AntWorldCell *simulationCell = environment.TryGetCellAtWorldPosition({2.75f, 2.25f});

    Require(simulationCell != nullptr, "A valid world position should resolve to a cell.");

    Require(simulationCell == environment.TryGetCell(2, 2), "World lookup should return the expected cell.");

    Require(environment.TryGetCellAtWorldPosition({1.0f, 1.0f}) == nullptr,
            "World lookup should reject positions in the border.");

    constexpr ColonyId TestColony{1};

    simulationCell->AddMarker(MarkerKind::ToFood, 100.0f, TestColony);

    simulationCell->SetPersistentMarker(MarkerKind::ToHome, 1'000.0f, TestColony);

    environment.Update(2.0f);

    Require(NearlyEqual(simulationCell->GetMarker(MarkerKind::ToFood).intensity, 93.0f),
            "Environment update should decay non-persistent markers.");

    Require(NearlyEqual(simulationCell->GetMarker(MarkerKind::ToHome).intensity, 1'000.0f),
            "Environment update should preserve persistent markers.");

    simulationCell->foodQuantity = 15;

    AntWorldCell *secondFoodCell = environment.TryGetCell(3, 2);

    Require(secondFoodCell != nullptr, "A second interior cell should exist.");

    secondFoodCell->foodQuantity = 7;

    Require(environment.GetTotalFoodQuantity() == 22, "Environment should report total food across all cells.");

    environment.Clear();

    Require(!environment.IsInitialized(), "Clear should reset environment initialization.");

    Require(environment.GetCellCount() == 0, "Clear should remove all cells.");

    AntConfiguration invalidConfiguration = configuration;

    invalidConfiguration.worldSize = {
        4,
        4,
    };

    invalidConfiguration.colonyPosition = {
        1.0f,
        1.0f,
    };

    Require(!environment.Initialize(invalidConfiguration, errorMessage),
            "A world containing only border cells should be rejected.");

    Require(!errorMessage.empty(), "Rejected environment initialization should explain the error.");

    std::cout << "All ant environment tests passed.\n";

    return 0;
}