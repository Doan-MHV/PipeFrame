#include "World/Runtime/AntQuery.h"
#include "Configuration/AntConfiguration.h"
#include "Editor/AntInspector.h"

#include <cstdlib>
#include <iostream>

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
    pipeframe::BehaviourScene antStoreScene;
    AntQuery antStore(antStoreScene);

    AntView ant =
        antStore.Create(
            7,
            AntRole::Explorer,
            {10.0f, 12.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId antId =
        ant.GetId();

    ant.Identity().nameIndex = 41;
    ant.Identity().nameSuffix = 2;

    ant.Motion().speed =
        configuration.antSpeed *
        0.5f;

    ant.Motion().travelDistance =
        125.0f;

    auto *foraging = antStore.GetWorld().Get<ForagingComponent>(antId);
    Require(foraging != nullptr, "Spawned ant has ECS foraging state.");
    foraging->collectedFood = 4;

    ant.SetTarget({
        20.0f,
        12.0f,
    });

    ant.SetState(
        ForagingState::ToHomeWithFood);

    AntInspector inspector(
        configuration);

    inspector.SetSelectedAnt(antId);
    inspector.SetFollowEnabled(true);
    inspector.SetHighlightEnabled(true);
    inspector.SetShowTargetEnabled(true);

    inspector.Refresh(
        antStore.GetAnts());

    const AntInspectorData &data =
        inspector.GetData();

    Require(
        data.available,
        "Selected ant data should be available.");

    Require(
        data.id == antId,
        "Inspector should expose selected ant ID.");

    Require(
        data.colonyId == 7,
        "Inspector should expose colony ID.");

    Require(
        data.displayName ==
            "Ant 42-2",
        "Inspector should expose the generated ant name.");

    Require(
        data.role == "Explorer",
        "Inspector should expose ant role.");

    Require(
        data.state ==
            "Returning with food",
        "Inspector should expose ant state.");

    Require(
        data.speedRatio == 0.5f,
        "Inspector should calculate normalized speed.");

    Require(
        data.totalTravelDistance ==
            125.0f,
        "Inspector should expose total distance.");

    Require(
        data.collectedFood == 4,
        "Inspector should expose collected food.");

    Require(
        data.carryingFood,
        "Returning-with-food ant should report carried food.");

    Require(
        data.follow &&
        data.highlight &&
        data.showTarget,
        "Inspector view options should be retained.");

    inspector.SetSelectedAnt(
        std::nullopt);

    Require(
        !inspector.GetData().available,
        "Clearing selection should clear inspector data.");

    std::cout
        << "All ant inspector tests passed.\n";

    return 0;
}
