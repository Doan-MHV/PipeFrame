#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/WallBuilder.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Require(
    const bool condition,
    const char *message
) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

bool NearlyEqual(
    const float first,
    const float second,
    const float tolerance = 0.0001f
) {
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
        5.0f,
        5.0f,
    };

    AntEnvironment environment;
    std::string errorMessage;

    Require(
        environment.Initialize(
            configuration,
            errorMessage),
        "Wall test environment should initialize.");

    Require(
        environment.GetWallCount() == 112,
        "A 16 by 16 world should create a two-cell border.");

    Require(
        environment.TryGetCell(0, 0)->wall,
        "Outer corner should be a border wall.");

    Require(
        environment.TryGetCell(1, 8)->wall,
        "Second border column should be a wall.");

    Require(
        !environment.TryGetCell(2, 8)->wall,
        "First interior column should not initially be a wall.");

    Require(
        NearlyEqual(
            environment
                .TryGetCell(2, 8)
                ->markerSamplingCoefficient,
            0.0f),
        "Cells adjacent to border walls should have zero marker sampling.");

    Require(
        NearlyEqual(
            environment
                .TryGetCell(3, 8)
                ->markerSamplingCoefficient,
            1.0f / 3.0f),
        "Cells two Manhattan units from walls should have reduced sampling.");

    Require(
        NearlyEqual(
            environment
                .TryGetCell(4, 8)
                ->markerSamplingCoefficient,
            1.0f),
        "Cells away from walls should have full marker sampling.");

    Require(
        environment.AddWall(
            {8.5f, 8.5f}),
        "An interior wall should be added.");

    Require(
        !environment.AddWall(
            {8.5f, 8.5f}),
        "Adding an existing wall should report no change.");

    Require(
        environment.TryGetCell(8, 8)->wall,
        "Added wall should be stored in its cell.");

    Require(
        NearlyEqual(
            environment
                .TryGetCell(8, 8)
                ->markerSamplingCoefficient,
            0.0f),
        "Wall cells should have zero marker sampling.");

    Require(
        NearlyEqual(
            environment
                .TryGetCell(9, 8)
                ->markerSamplingCoefficient,
            0.0f),
        "Cells adjacent to an obstacle should have zero marker sampling.");

    Require(
        NearlyEqual(
            environment
                .TryGetCell(10, 8)
                ->markerSamplingCoefficient,
            1.0f / 3.0f),
        "Cells two units from an obstacle should have reduced sampling.");

    Require(
        WallBuilder::IsWallBorder(
            environment,
            {8, 8}),
        "An isolated obstacle should be a physics border wall.");

    Require(
        WallBuilder::GetWallType(
            environment,
            {8, 8}) ==
            WallType::Full,
        "An isolated obstacle should use full wall geometry.");

    Require(
        environment.AddWall(
            {7.5f, 8.5f}),
        "Left neighboring wall should be added.");

    Require(
        environment.AddWall(
            {8.5f, 7.5f}),
        "Top neighboring wall should be added.");

    Require(
        WallBuilder::GetWallType(
            environment,
            {8, 8}) ==
            WallType::SouthEast,
        "Left and top neighbors should create a southeast corner.");

    Require(
        environment.RemoveWall(
            {8.5f, 7.5f}),
        "An interior wall should be removable.");

    Require(
        !environment.RemoveWall(
            {8.5f, 7.5f}),
        "Removing an empty cell should report no change.");

    Require(
        !environment.RemoveWall(
            {1.5f, 8.5f}),
        "The protected border should not be editable.");

    const std::size_t requestedWalls =
        environment.MarkWallBrush(
            {5.5f, 5.5f},
            1.1f);

    Require(
        requestedWalls == 5,
        "Radius 1.1 wall brush should request five cells.");

    Require(
        environment.TryGetCell(5, 5)
            ->IsWallRequested(),
        "Wall brush should defer its center edit.");

    Require(
        environment.ApplyWallRequests() == 5,
        "Applying wall requests should add five new walls.");

    Require(
        environment.TryGetCell(5, 5)->wall,
        "Applied wall request should create the center wall.");

    Require(
        !environment.TryGetCell(5, 5)
             ->IsWallRequested(),
        "Applied wall request should clear its edit state.");

    const std::size_t requestedErase =
        environment.MarkEraseBrush(
            {5.5f, 5.5f},
            1.1f);

    Require(
        requestedErase == 5,
        "Erase brush should request the five occupied cells.");

    Require(
        environment.ApplyEraseRequests() == 5,
        "Applying erase requests should remove five cells.");

    Require(
        !environment.TryGetCell(5, 5)->wall,
        "Erase request should remove the center wall.");

    const WorldEntityId foodId =
        environment.AddFood(
            {11.5f, 11.5f},
            10);

    Require(
        foodId != InvalidWorldEntityId,
        "Food should be added before combined erasing.");

    Require(
        environment.AddWall(
            {11.5f, 11.5f}),
        "A wall and food may occupy the same cell like AntPezza.");

    Require(
        environment.MarkCellForErase(
            {11.5f, 11.5f}),
        "Occupied cell should accept an erase request.");

    Require(
        environment.ApplyEraseRequests() == 1,
        "Combined wall and food cell should count as one erased cell.");

    const AntWorldCell *erasedCell =
        environment.TryGetCell(11, 11);

    Require(
        erasedCell != nullptr &&
            !erasedCell->wall &&
            erasedCell->foodQuantity == 0,
        "Erase should remove both wall and food.");

    Require(
        environment.FindFoodEntity(foodId) == nullptr,
        "Erasing food should remove its render entity.");

    Require(
        !environment.MarkCellForErase(
            {10.5f, 10.5f}),
        "An empty cell should not accept an erase request.");

    Require(
        !environment.MarkCellForWall(
            {1.5f, 8.5f}),
        "Border cells should not accept editor requests.");

    std::cout << "All ant wall-builder tests passed.\n";

    return 0;
}
