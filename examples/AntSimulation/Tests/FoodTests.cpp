#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/Food.h"

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

} // namespace

int main() {
    using namespace ant_simulation;

    AntConfiguration configuration;

    configuration.worldSize = {
        12,
        12,
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
        "Food test environment should initialize.");

    const WorldEntityId firstFoodId =
        environment.AddFood(
            {4.2f, 4.8f},
            5);

    Require(
        firstFoodId != InvalidWorldEntityId,
        "Adding food should create a food entity.");

    AntWorldCell *firstCell =
        environment.TryGetCell(4, 4);

    Require(
        firstCell != nullptr,
        "Food cell should exist.");

    Require(
        firstCell->foodQuantity == 5,
        "Food quantity should be stored in its world cell.");

    Require(
        firstCell->foodEntityId == firstFoodId,
        "Food cell should reference its render entity.");

    Require(
        environment.GetFoodEntities().size() == 1,
        "First food cell should create one render entity.");

    const Food *firstFood =
        environment.FindFoodEntity(firstFoodId);

    Require(
        firstFood != nullptr,
        "Created food entity should be retrievable.");

    Require(
        firstFood->position ==
            pipeframe::Vector2f{4.5f, 4.5f},
        "Food render entity should use the cell center.");

    const WorldEntityId repeatedFoodId =
        environment.AddFood(
            {4.9f, 4.1f},
            3);

    Require(
        repeatedFoodId == firstFoodId,
        "Adding to an occupied cell should reuse its entity.");

    Require(
        firstCell->foodQuantity == 8,
        "Adding to an occupied cell should increase its quantity.");

    Require(
        environment.GetFoodEntities().size() == 1,
        "Adding to an occupied cell should not create another entity.");

    Require(
        environment.ConsumeFood(
            {4.0f, 4.0f},
            3) == 3,
        "ConsumeFood should return the consumed quantity.");

    Require(
        firstCell->foodQuantity == 5,
        "Partial consumption should leave remaining food.");

    Require(
        environment.FindFoodEntity(firstFoodId) != nullptr,
        "Partially consumed food should retain its entity.");

    Require(
        environment.ConsumeFood(
            {4.0f, 4.0f},
            100) == 5,
        "Consumption should be limited to available food.");

    Require(
        firstCell->foodQuantity == 0,
        "Consuming all food should empty its cell.");

    Require(
        firstCell->foodEntityId ==
            InvalidWorldEntityId,
        "Empty food cells should release their entity ID.");

    Require(
        environment.FindFoodEntity(firstFoodId) == nullptr,
        "Consumed food entity should be removed.");

    Require(
        environment.GetFoodEntities().empty(),
        "No food render entities should remain.");

    Require(
        environment.AddFood(
            {1.0f, 1.0f},
            5) == InvalidWorldEntityId,
        "Food cannot be added inside the physics border.");

    Require(
        environment.AddFood(
            {4.0f, 4.0f},
            0) == InvalidWorldEntityId,
        "Adding zero food should do nothing.");

    const std::size_t modifiedCells =
        environment.AddFoodPatch(
            {6.0f, 6.0f},
            2.1f,
            3);

    Require(
        modifiedCells == 13,
        "A radius 2.1 patch should affect thirteen grid cells.");

    Require(
        environment.GetFoodEntities().size() == 13,
        "Each food patch cell should have one render entity.");

    Require(
        environment.GetTotalFoodQuantity() == 39,
        "Patch quantity should be added to every affected cell.");

    const AntWorldCell *patchCenter =
        environment.TryGetCell(6, 6);

    Require(
        patchCenter != nullptr &&
            patchCenter->foodQuantity == 3,
        "Food patch should include its center cell.");

    const WorldEntityId patchCenterId =
        patchCenter->foodEntityId;

    Require(
        environment.RemoveFood(
            {6.2f, 6.7f}),
        "RemoveFood should remove an occupied cell.");

    Require(
        environment.FindFoodEntity(
            patchCenterId) == nullptr,
        "Removing food should remove its render entity.");

    Require(
        environment.GetFoodEntities().size() == 12,
        "Removing one food cell should leave twelve entities.");

    Require(
        environment.GetTotalFoodQuantity() == 36,
        "Removing one patch cell should remove its quantity.");

    Require(
        !environment.RemoveFood(
            {6.2f, 6.7f}),
        "Removing an empty food cell should return false.");

    environment.ClearAllFood();

    Require(
        environment.GetFoodEntities().empty(),
        "ClearAllFood should remove all food entities.");

    Require(
        environment.GetTotalFoodQuantity() == 0,
        "ClearAllFood should empty all food cells.");

    for (const AntWorldCell &cell :
         environment.GetCells()) {
        Require(
            cell.foodQuantity == 0,
            "Every cell should be empty after ClearAllFood.");

        Require(
            cell.foodEntityId ==
                InvalidWorldEntityId,
            "Every cell should release its food entity ID.");
    }

    std::cout << "All ant food tests passed.\n";

    return 0;
}
