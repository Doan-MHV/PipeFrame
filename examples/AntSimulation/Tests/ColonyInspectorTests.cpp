#include "ColonyFixture.h"
#include "World/Runtime/ColonyView.h"
#include "Configuration/AntConfiguration.h"
#include "Editor/ColonyInspector.h"

#include <array>
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

    ColonyFixture fixture{
            11,
            {10.0f, 20.0f},
            {239, 71, 111},
            configuration,
        };
    std::array<ColonyView, 1> colonies{fixture};

    colonies[0].SetAntCount(250);
    colonies[0].AddFood(50.0f);
    colonies[0].UpdateCollectionRate();
    colonies[0].AddFood(15.0f);
    colonies[0].UpdateCollectionRate();

    ColonyInspector inspector;

    inspector.SetSelectedColony(11);
    inspector.Refresh(colonies);

    Require(
        inspector.GetData().available,
        "Selected colony data should be available.");

    Require(
        inspector.GetData().id == 11,
        "Inspector should expose colony ID.");

    Require(
        inspector.GetData().antCount ==
            250,
        "Inspector should expose colony population.");

    Require(
        inspector.GetData().foodQuantity ==
            65.0f,
        "Inspector should expose collected food.");

    Require(
        inspector.SetPosition(
            colonies,
            {30.0f, 40.0f}),
        "Position should be editable.");

    Require(
        colonies[0].GetPosition() ==
            pipeframe::Vector2f{
                30.0f,
                40.0f,
            },
        "Edited position should reach colony.");

    Require(
        inspector.SetRadius(
            colonies,
            12.0f),
        "Radius should be editable.");

    Require(
        colonies[0].GetRadius() ==
            12.0f,
        "Edited radius should reach colony.");

    Require(
        inspector.SetReserve(
            colonies,
            500.0f),
        "Reserve should be editable.");

    Require(
        colonies[0].GetReserve() ==
            500.0f,
        "Edited reserve should reach colony.");

    Require(
        inspector.SetSoldierRequested(
            colonies,
            3.0f),
        "Soldier request should be editable.");

    Require(
        colonies[0].State().soldierRequested ==
            3.0f,
        "Edited soldier request should reach colony.");

    Require(
        !inspector.SetRadius(
            colonies,
            -1.0f),
        "Negative radius should be rejected.");

    inspector.SetSelectedColony(
        std::nullopt);

    inspector.Refresh(colonies);

    Require(
        !inspector.GetData().available,
        "Clearing selection should clear colony data.");

    std::cout
        << "All colony inspector tests passed.\n";

    return 0;
}
