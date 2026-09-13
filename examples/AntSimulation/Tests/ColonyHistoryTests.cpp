#include "ColonyFixture.h"
#include <cstdlib>
#include <iostream>
#include <string>

#include "World/Runtime/ColonyHistory.h"

namespace {

void Require(
    const bool condition,
    const std::string &message
) {
    if (condition) {
        return;
    }

    std::cerr
        << "FAILED: "
        << message
        << '\n';

    std::exit(1);
}

} // namespace

int main() {
    using namespace ant_simulation;

    AntConfiguration configuration;

    configuration.colonyRadius = 6.0f;
    configuration.colonyInitialAntCount = 100;
    configuration.antCost = 2.0f;

    ColonyFixture colony(
        1,
        {20.0f, 30.0f},
        pipeframe::Color::Red,
        configuration
    );

    colony.SetAntCount(100);
    colony.SetReserve(200.0f);

    ColonyHistory history(
        3,
        0.1f
    );

    Require(
        history.GetSamples().empty(),
        "New history should be empty."
    );

    history.Update(
        colony,
        0.05f
    );

    Require(
        history.GetSamples().empty(),
        "A partial sample period should not create a sample."
    );

    history.Update(
        colony,
        0.05f
    );

    Require(
        history.GetSamples().size() == 1,
        "One complete sample period should create one sample."
    );

    Require(
        history.GetSamples().back().antCount == 100,
        "History should record the colony population."
    );

    Require(
        history.GetSamples().back().reserve == 200.0f,
        "History should record the colony reserve."
    );

    colony.SetAntCount(120);
    colony.SetReserve(250.0f);
    colony.AddFood(10.0f);
    colony.UpdateCollectionRate();

    history.Update(
        colony,
        0.2f
    );

    Require(
        history.GetSamples().size() == 3,
        "Multiple elapsed periods should create multiple samples."
    );

    Require(
        history.GetSamples().back().antCount == 120,
        "New samples should contain the current population."
    );

    Require(
        history.GetSamples().back().foodQuantity == 10.0f,
        "New samples should contain collected food."
    );

    colony.SetAntCount(140);

    history.Update(
        colony,
        0.1f
    );

    Require(
        history.GetSamples().size() == 3,
        "History should enforce its maximum capacity."
    );

    Require(
        history.GetSamples().back().antCount == 140,
        "The newest sample should remain after capacity trimming."
    );

    history.Reset();

    Require(
        history.GetSamples().empty(),
        "Reset should remove all samples."
    );

    std::cout
        << "All colony history tests passed.\n";

    return 0;
}
