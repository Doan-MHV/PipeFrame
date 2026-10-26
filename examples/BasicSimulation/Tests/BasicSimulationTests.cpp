#include "BasicSimulationRuntime.h"
#include "BasicSimulationTypes.h"
#include "Population.h"
#include "PopulationLod.h"

#include <cstdlib>
#include <iostream>
#include <span>
#include <vector>

namespace {

void Require(
    const bool condition,
    const char *message) {

    if (!condition) {
        std::cerr
            << "FAILED: "
            << message << '\n';

        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace basic_simulation;

    BasicSimulationRuntime runtime;

    std::string errorMessage;

    Require(
        runtime.Load({}, errorMessage),
        "Runtime should load.");

    const auto types =
        runtime.GetSceneObjectTypes();

    Require(
        types.size() == 2,
        "Runtime should expose two object types.");

    Require(
        types[0].typeId == DemoAgentTypeId,
        "First type should be demo agent.");

    Require(
        types[1].typeId == PopulationTypeId,
        "Second type should be population.");

    pipeframe::SceneObjectData agent =
        runtime.CreateDefaultObject(
            DemoAgentTypeId);

    agent.id = 1;
    agent.transform.position = {
        40.0f,
        25.0f,
    };

    pipeframe::SceneObjectData populationObject =
        runtime.CreateDefaultObject(
            PopulationTypeId);

    populationObject.id = 2;
    populationObject.transform.position = {
        500.0f,
        500.0f,
    };

    populationObject.properties[AgentCountKey] =
        std::int64_t{1000};

    populationObject.properties[SpawnAreaKey] =
        pipeframe::Vector2f{
            1000.0f,
            1000.0f,
        };

    populationObject.properties[RandomSeedKey] =
        std::int64_t{7};

    std::vector<pipeframe::SceneObjectData> objects{
        agent,
        populationObject,
    };

    runtime.SynchronizeScene(objects);

    const auto agentHit =
        runtime.HitTest({
            40.0f,
            25.0f,
        });

    Require(
        agentHit.has_value() &&
            *agentHit == 1,
        "Agent should be selectable.");

    const auto populationHit =
        runtime.HitTest({
            900.0f,
            900.0f,
        });

    Require(
        populationHit.has_value() &&
            *populationHit == 2,
        "Population bounds should be selectable.");

    Population population;

    Require(
        population.Initialize(
            populationObject),
        "Population should initialize.");

    Require(
        population.GetCount() == 1000,
        "Population should contain 1000 agents.");

    Require(
        population.MatchesSource(
            populationObject),
        "Population should match its source.");

    population.FixedUpdate(1.0f / 60.0f);

    for (int index = 0; index < 8; ++index) {
        population.FixedUpdate(
            1.0f / 60.0f);
    }

    population.RebuildSpatialGrid();

    Require(
        SelectPopulationRenderMode(
            PopulationRenderMode::Points,
            0.20f) ==
            PopulationRenderMode::Quads,
        "Close zoom should use quads.");

    Require(
        SelectPopulationRenderMode(
            PopulationRenderMode::Quads,
            0.60f) ==
            PopulationRenderMode::Points,
        "Distant zoom should use points.");

    runtime.SetSelectedObject(1);
    runtime.Start();
    runtime.FixedUpdate(1.0f / 60.0f);
    runtime.Stop();
    runtime.Reset();
    runtime.Unload();

    std::cout
        << "All basic simulation tests passed.\n";

    return 0;
}
