#include <cmath>
#include <array>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>

#include "Configuration/SailBoatConfiguration.h"
#include "Runtime/SailBoatSimulationRuntime.h"
#include "Components/SailBoatSimulationTypes.h"
#include "World/RaceCourse.h"
#include "World/RaceCoursePersistence.h"
#include "World/RaceSegment.h"

namespace {

bool NearlyEqual(const float left, const float right, const float tolerance = 0.001f) {
    return std::abs(left - right) <= tolerance;
}

bool Check(const bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool TestConfiguration() {
    sailboat_simulation::SailBoatConfiguration configuration;
    std::string errorMessage;
    bool passed = true;

    passed &= Check(configuration.Validate(errorMessage), "Default SailBoat configuration should be valid.");
    passed &= Check(configuration.populationSize == 1000, "Default population must match SailBoatPezza.");
    passed &= Check(NearlyEqual(configuration.simulationSpeedUp, 10.0f),
                    "Default training speed must match SailBoatPezza.");

    configuration.eliteRatio = 1.5f;
    passed &= Check(!configuration.Validate(errorMessage), "Invalid evolution ratios should be rejected.");
    return passed;
}

bool TestRaceGeometry() {
    using namespace sailboat_simulation;

    const RaceSegment segment({10.0f, 20.0f}, {110.0f, 20.0f});
    bool passed = true;
    passed &= Check(segment.IsValid(), "A non-zero race segment should be valid.");
    passed &= Check(NearlyEqual(segment.GetLength(), 100.0f), "Race segment length is incorrect.");
    passed &= Check(NearlyEqual(segment.DistanceTo({60.0f, 35.0f}, false), 15.0f),
                    "Race segment projection distance is incorrect.");

    const RaceSegment zeroLength({4.0f, 4.0f}, {4.0f, 4.0f});
    passed &= Check(!zeroLength.IsValid(), "Zero-length race geometry must be rejected safely.");

    RaceCourse course;
    course.SetStart(segment);
    course.SetFinish({{200.0f, 0.0f}, {200.0f, 100.0f}});
    course.AddWaypoint({{{150.0f, 100.0f}, {175.0f, 100.0f}}, 10.0f, 2});
    course.AddWaypoint({{{100.0f, 100.0f}, {125.0f, 100.0f}}, 10.0f, 1});
    course.SortWaypoints();

    std::string errorMessage;
    passed &= Check(course.Validate(errorMessage), "Complete race course should validate.");
    passed &= Check(course.GetTargetCount() == 3, "Waypoints plus finish should define all race targets.");
    passed &= Check(NearlyEqual(course.GetWaypoints().front().segment.GetFirstPoint().x, 100.0f),
                    "Waypoints should be sorted by authored race order.");
    return passed;
}

bool TestRacePersistence() {
    using namespace sailboat_simulation;

    pipeframe::SceneObjectData start;
    start.name = "RACE START";
    start.typeId = RaceStartTypeId;
    start.transform = {{12.0f, 34.0f}, 27.0f};
    start.properties.emplace(LineLengthKey, 91.0);

    pipeframe::SceneObjectData waypoint;
    waypoint.name = "WAYPOINT 3";
    waypoint.typeId = WaypointTypeId;
    waypoint.transform = {{56.0f, 78.0f}, -14.0f};
    waypoint.properties.emplace(LineLengthKey, 62.0);
    waypoint.properties.emplace(WaypointRadiusKey, 13.0);
    waypoint.properties.emplace(WaypointOrderKey, std::int64_t{3});

    pipeframe::SceneObjectData environment;
    environment.name = "ENVIRONMENT";
    environment.typeId = EnvironmentTypeId;

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "pipeframe_sailboat_race_test.pfrace";
    std::string errorMessage;
    bool passed = RaceCoursePersistence::Save(path, std::array{start, waypoint, environment}, errorMessage);
    passed &= Check(passed, "Race editor should save authored race objects. " + errorMessage);

    std::vector<pipeframe::SceneObjectData> loaded;
    const bool loadedOk = RaceCoursePersistence::Load(path, loaded, errorMessage);
    passed &= Check(loadedOk, "Race editor should load its saved race. " + errorMessage);
    passed &= Check(loaded.size() == 2, "Race files must exclude environment and training settings.");
    if (loaded.size() == 2) {
        passed &= Check(loaded[0].transform == start.transform,
                        "Race save/load should preserve start geometry.");
        passed &= Check(std::get<std::int64_t>(loaded[1].properties.at(WaypointOrderKey)) == 3,
                        "Race save/load should preserve waypoint ordering.");
    }
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    return passed;
}

bool TestRuntimeContract() {
    using namespace sailboat_simulation;

    SailBoatSimulationRuntime runtime;
    std::string errorMessage;
    bool passed = true;
    passed &= Check(runtime.Load({PIPEFRAME_SAILBOAT_ASSET_ROOT}, errorMessage),
                    "SailBoat runtime foundation should load.");
    passed &= Check(runtime.GetSceneObjectTypes().size() == 5,
                    "Runtime must expose start, finish, waypoint, environment, and training types.");
    passed &= Check(runtime.GetSceneComponentTypes().size() == 5,
                    "Every SailBoat domain object type must expose a component schema.");

    auto start = runtime.CreateDefaultObject(RaceStartTypeId);
    auto finish = runtime.CreateDefaultObject(FinishLineTypeId);
    auto waypoint = runtime.CreateDefaultObject(WaypointTypeId);
    auto environment = runtime.CreateDefaultObject(EnvironmentTypeId);
    auto training = runtime.CreateDefaultObject(TrainingSettingsTypeId);

    start.id = 1;
    finish.id = 2;
    waypoint.id = 3;
    environment.id = 4;
    training.id = 5;

    const auto environmentComponent = std::ranges::find(
        environment.components, EnvironmentTypeId, &pipeframe::SceneComponentData::typeId);
    passed &= Check(environmentComponent != environment.components.end(),
                    "Environment settings must be stored in a project component.");
    for(const auto *key:{WorldSizeKey,WindDirectionKey,WindSpeedKey,WaterAnimationKey,DrawBestOnlyKey,
                        DrawGhostBoatsKey,HighlightBestKey,DrawBestTrajectoryKey,DrawWaypointLabelsKey,
                        ShowTargetGuideKey,AudioEnabledKey,AudioVolumeKey}) {
        passed &= Check(environmentComponent != environment.components.end() &&
                            environmentComponent->properties.contains(key),
                        std::string("Missing environment component control: ")+key);
    }

    runtime.SynchronizeScene(std::array{start, finish, waypoint, environment, training});

    std::string courseError;
    passed &= Check(runtime.GetRaceCourse().Validate(courseError), "Default authored race should be valid.");
    passed &= Check(runtime.GetConfiguration().populationSize == 1000,
                    "Training settings should populate the runtime configuration.");
    passed &= Check(runtime.GetConfiguration().drawGhostBoats &&
                        runtime.GetConfiguration().highlightBest &&
                        runtime.GetConfiguration().drawBestTrajectory &&
                        runtime.GetConfiguration().drawWaypointLabels &&
                        runtime.GetConfiguration().showTargetGuide &&
                        !runtime.GetConfiguration().drawBestOnly,
                    "Pezza visual controls should use useful project defaults.");
    passed &= Check(runtime.HitTest(start.transform.position).value_or(0) == start.id,
                    "Race start should be selectable in the Workbench viewport.");
    passed &= Check(runtime.HitTest(waypoint.transform.position).value_or(0) == waypoint.id,
                    "Waypoint should be selectable in the Workbench viewport.");
    passed &= Check(runtime.HasPreviewBoat(), "A valid race start should create the preview boat.");
    passed &= Check(runtime.GetPopulationTrainer().IsInitialized(),
                    "A valid authored race should initialize population training.");
    passed &= Check(runtime.GetPopulationTrainer().GetAgents().size() == 1000,
                    "Runtime training should create the configured 1,000-agent population.");

    const pipeframe::Vector2f initialBoatPosition = runtime.GetPreviewBoat().GetPosition();

    runtime.Start();
    passed &= Check(runtime.IsPlaying(), "Runtime should enter its playing state.");
    runtime.FixedUpdate(1.0f);
    passed &= Check(runtime.GetPopulationTrainer().GetGenerationTime() > 0.0f,
                    "Playing the runtime should advance population evaluation.");
    passed &= Check(runtime.GetPreviewBoat().GetPosition() != initialBoatPosition,
                    "The runtime preview boat should move using the authored wind and start heading.");
    passed &= Check(runtime.GetPreviewRaceTask().GetWorldTime() == 1.0f,
                    "The runtime should advance its race task with the preview boat.");
    passed &= Check(runtime.GetPreviewRaceTask().BuildNeuralInputs(
                        runtime.GetPreviewBoat(),
                        {runtime.GetConfiguration().worldSize, runtime.GetConfiguration().wind},
                        runtime.GetRaceCourse()).size() == 4,
                    "The runtime race task should expose Pezza's four-input network contract.");
    runtime.Stop();
    passed &= Check(!runtime.IsPlaying(), "Runtime should leave its playing state.");
    runtime.Reset();
    passed &= Check(runtime.GetPopulationTrainer().GetGeneration() == 0 &&
                        runtime.GetPopulationTrainer().GetGenerationTime() == 0.0f,
                    "Reset should restart population training from generation zero.");
    passed &= Check(runtime.GetPreviewBoat().GetPosition() == initialBoatPosition,
                    "Reset should restore the preview boat to the authored race start.");
    passed &= Check(runtime.GetPreviewRaceTask().GetTargetIndex() == 0 &&
                        runtime.GetPreviewRaceTask().GetWorldTime() == 0.0f,
                    "Reset should restore race progress to the first target.");
    runtime.Unload();
    return passed;
}

} // namespace

int main() {
    bool passed = true;
    passed &= TestConfiguration();
    passed &= TestRaceGeometry();
    passed &= TestRacePersistence();
    passed &= TestRuntimeContract();

    if (!passed) {
        return 1;
    }

    std::cout << "All SailBoat foundation tests passed.\n";
    return 0;
}
