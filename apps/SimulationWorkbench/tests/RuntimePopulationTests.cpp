#include "../Editor/SceneTypes.h"
#include "../Runtime/RuntimeAgentPopulation.h"
#include "../Runtime/RuntimePopulationLod.h"
#include "../Runtime/RuntimePopulationSpatialGrid.h"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using pipeframe::editor::AgentPopulationSettings;
using pipeframe::editor::SceneObjectData;
using pipeframe::editor::SceneObjectType;

using pipeframe::runtime::RuntimeAgentPopulation;
using pipeframe::runtime::RuntimePopulationLodThresholds;
using pipeframe::runtime::RuntimePopulationRenderMode;
using pipeframe::runtime::RuntimePopulationSpatialGrid;
using pipeframe::runtime::SelectRuntimePopulationRenderMode;

bool Check(const bool condition, const std::string &message) {

    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    return true;
}

SceneObjectData MakePopulationObject(const std::uint32_t count, const sf::Vector2f spawnArea,
                                     const std::uint32_t seed) {

    SceneObjectData object;
    object.id = 1;
    object.name = "TEST POPULATION";
    object.type = SceneObjectType::AgentPopulation;
    object.transform.position = {25.0f, -40.0f};
    object.transform.rotation = 15.0f;

    AgentPopulationSettings settings;
    settings.agentCount = count;
    settings.spawnAreaSize = spawnArea;
    settings.randomSeed = seed;

    object.population = settings;
    return object;
}

bool TestLodHysteresis() {
    bool passed = true;

    const RuntimePopulationLodThresholds thresholds{
        0.30f,
        0.45f,
    };

    passed &= Check(SelectRuntimePopulationRenderMode(RuntimePopulationRenderMode::Points, 1.00f, thresholds) ==
                        RuntimePopulationRenderMode::Points,
                    "Points should remain active at normal zoom.");

    passed &= Check(SelectRuntimePopulationRenderMode(RuntimePopulationRenderMode::Points, 0.30f, thresholds) ==
                        RuntimePopulationRenderMode::Quads,
                    "Points should switch to quads at the quad threshold.");

    passed &= Check(SelectRuntimePopulationRenderMode(RuntimePopulationRenderMode::Quads, 0.35f, thresholds) ==
                        RuntimePopulationRenderMode::Quads,
                    "Quads should remain active inside the hysteresis band.");

    passed &= Check(SelectRuntimePopulationRenderMode(RuntimePopulationRenderMode::Points, 0.35f, thresholds) ==
                        RuntimePopulationRenderMode::Points,
                    "Points should remain active inside the hysteresis band.");

    passed &= Check(SelectRuntimePopulationRenderMode(RuntimePopulationRenderMode::Quads, 0.45f, thresholds) ==
                        RuntimePopulationRenderMode::Points,
                    "Quads should switch to points at the point threshold.");

    passed &= Check(SelectRuntimePopulationRenderMode(RuntimePopulationRenderMode::Points,
                                                      std::numeric_limits<float>::quiet_NaN(),
                                                      thresholds) == RuntimePopulationRenderMode::Points,
                    "Invalid zoom must not change the render mode.");

    const RuntimePopulationLodThresholds invalidThresholds{
        0.50f,
        0.40f,
    };

    passed &= Check(SelectRuntimePopulationRenderMode(RuntimePopulationRenderMode::Quads, 1.00f, invalidThresholds) ==
                        RuntimePopulationRenderMode::Quads,
                    "Invalid thresholds must not change the render mode.");

    return passed;
}

bool TestSpatialGridQueries() {
    RuntimePopulationSpatialGrid grid;

    grid.Initialize({-100.0f, -100.0f}, {200.0f, 200.0f}, 50.0f, 5);

    const std::vector<float> positionX{
        -75.0f, -25.0f, 25.0f, 75.0f, 0.0f,
    };

    const std::vector<float> positionY{
        -75.0f, -25.0f, 25.0f, 75.0f, 0.0f,
    };

    grid.Rebuild(positionX, positionY);

    bool passed = true;

    passed &= Check(grid.GetColumnCount() == 4, "Expected four spatial-grid columns.");

    passed &= Check(grid.GetRowCount() == 4, "Expected four spatial-grid rows.");

    const RuntimePopulationSpatialGrid::CellRange centerRange = grid.GetCellsOverlapping({
        {-10.0f, -10.0f},
        {20.0f, 20.0f},
    });

    passed &= Check(!centerRange.IsEmpty(), "Center query should overlap the grid.");

    std::size_t centerCandidateCount = 0;

    if (!centerRange.IsEmpty()) {
        for (int row = centerRange.minimumRow; row <= centerRange.maximumRow; ++row) {

            for (int column = centerRange.minimumColumn; column <= centerRange.maximumColumn; ++column) {

                centerCandidateCount += grid.GetAgentIndices(column, row).size();
            }
        }
    }

    passed &= Check(centerCandidateCount >= 1, "Center query should return at least one candidate.");

    const RuntimePopulationSpatialGrid::CellRange outsideRange = grid.GetCellsOverlapping({
        {500.0f, 500.0f},
        {50.0f, 50.0f},
    });

    passed &= Check(outsideRange.IsEmpty(), "Outside query should be empty.");

    passed &= Check(grid.GetAgentIndices(-1, 0).empty(), "Negative column should return an empty span.");

    passed &= Check(grid.GetAgentIndices(0, -1).empty(), "Negative row should return an empty span.");

    passed &= Check(grid.GetAgentIndices(100, 100).empty(), "Out-of-range cell should return an empty span.");

    return passed;
}

bool TestSpatialGridInputSafety() {
    RuntimePopulationSpatialGrid grid;

    grid.Initialize({-50.0f, -50.0f}, {100.0f, 100.0f}, 25.0f, 4);

    const std::vector<float> positionX{
        -20.0f,
        0.0f,
        20.0f,
        40.0f,
    };

    // Deliberately shorter than positionX.
    const std::vector<float> positionY{
        -20.0f,
        0.0f,
    };

    grid.Rebuild(positionX, positionY);

    bool passed = true;

    std::size_t totalStoredAgents = 0;

    for (int row = 0; row < grid.GetRowCount(); ++row) {

        for (int column = 0; column < grid.GetColumnCount(); ++column) {

            totalStoredAgents += grid.GetAgentIndices(column, row).size();
        }
    }

    passed &= Check(totalStoredAgents == 2, "Grid should use the smaller position-array size.");

    const float invalid = std::numeric_limits<float>::quiet_NaN();

    passed &= Check(grid.GetCellsOverlapping({
                                                 {invalid, 0.0f},
                                                 {10.0f, 10.0f},
                                             })
                        .IsEmpty(),
                    "NaN query should be rejected.");

    passed &= Check(grid.GetCellsOverlapping({
                                                 {0.0f, 0.0f},
                                                 {-10.0f, 10.0f},
                                             })
                        .IsEmpty(),
                    "Negative query size should be rejected.");

    grid.Clear();

    passed &= Check(grid.GetColumnCount() == 0, "Clear should reset the column count.");

    passed &= Check(grid.GetRowCount() == 0, "Clear should reset the row count.");

    passed &= Check(grid.GetCellsOverlapping({
                                                 {0.0f, 0.0f},
                                                 {10.0f, 10.0f},
                                             })
                        .IsEmpty(),
                    "Cleared grid should return an empty query.");

    return passed;
}

bool TestPopulationDeterministicReset() {
    const SceneObjectData source = MakePopulationObject(4096, {1000.0f, 800.0f}, 12345);

    RuntimeAgentPopulation firstPopulation;
    RuntimeAgentPopulation secondPopulation;

    bool passed = true;

    passed &= Check(firstPopulation.Initialize(source), "First deterministic population failed to initialize.");

    passed &= Check(secondPopulation.Initialize(source), "Second deterministic population failed to initialize.");

    if (!passed) {
        return false;
    }

    passed &= Check(firstPopulation.GetCount() == 4096, "Population count was not preserved.");

    passed &= Check(firstPopulation.GetPositionX() == secondPopulation.GetPositionX(),
                    "Equal seeds should produce equal X positions.");

    passed &= Check(firstPopulation.GetPositionY() == secondPopulation.GetPositionY(),
                    "Equal seeds should produce equal Y positions.");

    const std::vector<float> originalX = firstPopulation.GetPositionX();

    const std::vector<float> originalY = firstPopulation.GetPositionY();

    firstPopulation.Update(1.0f / 60.0f);

    passed &= Check(firstPopulation.GetPositionX() != originalX || firstPopulation.GetPositionY() != originalY,
                    "A simulation update should move agents.");

    firstPopulation.RebuildSpatialGrid();

    firstPopulation.Clear();

    passed &= Check(firstPopulation.GetCount() == 0, "Clear should remove all runtime agents.");

    passed &= Check(firstPopulation.Initialize(source), "Population failed to reinitialize after reset.");

    passed &= Check(firstPopulation.GetPositionX() == originalX, "Reset should reproduce deterministic X positions.");

    passed &= Check(firstPopulation.GetPositionY() == originalY, "Reset should reproduce deterministic Y positions.");

    return passed;
}

bool TestPopulationValidation() {
    bool passed = true;

    RuntimeAgentPopulation population;

    SceneObjectData wrongType;
    wrongType.type = SceneObjectType::DemoAgent;

    passed &= Check(!population.Initialize(wrongType), "A demo agent must not initialize as a population.");

    SceneObjectData zeroCount = MakePopulationObject(0, {100.0f, 100.0f}, 1);

    passed &= Check(!population.Initialize(zeroCount), "A zero-agent runtime population should be rejected.");

    SceneObjectData zeroWidth = MakePopulationObject(10, {0.0f, 100.0f}, 1);

    passed &= Check(!population.Initialize(zeroWidth), "A zero-width spawn area should be rejected.");

    SceneObjectData tooLarge = MakePopulationObject(1'000'001, {1000.0f, 1000.0f}, 1);

    passed &= Check(!population.Initialize(tooLarge), "Runtime populations above one million should be rejected.");

    return passed;
}

bool TestMillionAgentSmoke() {
    using Clock = std::chrono::steady_clock;

    const SceneObjectData source = MakePopulationObject(1'000'000, {4000.0f, 4000.0f}, 1);

    RuntimeAgentPopulation population;

    const auto initializeStart = Clock::now();

    if (!population.Initialize(source)) {
        std::cerr << "FAILED: Million-agent population "
                     "failed to initialize.\n";
        return false;
    }

    const auto initializeEnd = Clock::now();

    const auto updateStart = Clock::now();

    // One complete movement-slice cycle.
    for (int step = 0; step < 8; ++step) {
        population.Update(1.0f / 60.0f);
    }

    population.RebuildSpatialGrid();

    const auto updateEnd = Clock::now();

    const float initializeTimeMs = std::chrono::duration<float, std::milli>(initializeEnd - initializeStart).count();

    const float updateCycleTimeMs = std::chrono::duration<float, std::milli>(updateEnd - updateStart).count();

    std::cout << "Million-agent initialization: " << initializeTimeMs << " ms\n"
              << "Million-agent 8-tick cycle and grid rebuild: " << updateCycleTimeMs << " ms\n";

    bool passed = true;

    passed &= Check(population.GetCount() == 1'000'000, "Million-agent population count changed.");

    passed &= Check(population.GetPositionX().size() == 1'000'000, "Million-agent X storage has the wrong size.");

    passed &= Check(population.GetPositionY().size() == 1'000'000, "Million-agent Y storage has the wrong size.");

    passed &= Check(population.GetSpatialGrid().GetColumnCount() > 0, "Million-agent spatial grid has no columns.");

    passed &= Check(population.GetSpatialGrid().GetRowCount() > 0, "Million-agent spatial grid has no rows.");

    return passed;
}

} // namespace

int main() {
    bool passed = true;

    passed &= TestLodHysteresis();
    passed &= TestSpatialGridQueries();
    passed &= TestSpatialGridInputSafety();
    passed &= TestPopulationDeterministicReset();
    passed &= TestPopulationValidation();
    passed &= TestMillionAgentSmoke();

    if (!passed) {
        return 1;
    }

    std::cout << "All runtime population tests passed.\n";

    return 0;
}