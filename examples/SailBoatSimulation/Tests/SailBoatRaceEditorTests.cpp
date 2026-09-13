#include <array>
#include <filesystem>
#include <iostream>

#include "Components/SailBoatSimulationTypes.h"
#include "World/RaceCoursePersistence.h"

namespace {

bool Check(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

} // namespace

int main() {
    using namespace sailboat_simulation;

    pipeframe::SceneObjectData start;
    start.name = "RACE START";
    start.typeId = RaceStartTypeId;
    start.transform = {{10.0f, 20.0f}, 45.0f};
    start.properties.emplace(LineLengthKey, 75.0);

    pipeframe::SceneObjectData mark;
    mark.name = "WAYPOINT 4";
    mark.typeId = WaypointTypeId;
    mark.transform = {{300.0f, 400.0f}, -20.0f};
    mark.properties.emplace(LineLengthKey, 60.0);
    mark.properties.emplace(WaypointRadiusKey, 12.0);
    mark.properties.emplace(WaypointOrderKey, std::int64_t{4});

    pipeframe::SceneObjectData settings;
    settings.name = "TRAINING SETTINGS";
    settings.typeId = TrainingSettingsTypeId;

    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "pipeframe_sailboat_15g";
    const std::filesystem::path file = root / "race.pfrace";
    std::string error;
    bool passed = true;
    passed &= Check(RaceCoursePersistence::Save(file, std::array{start, mark, settings}, error),
                    "15G race save should succeed.");

    std::vector<pipeframe::SceneObjectData> loaded;
    passed &= Check(RaceCoursePersistence::Load(file, loaded, error),
                    "15G race load should succeed.");
    passed &= Check(loaded.size() == 2, "Race persistence must not overwrite project settings.");
    if (loaded.size() == 2) {
        passed &= Check(loaded[0].transform == start.transform, "Start geometry must round-trip.");
        passed &= Check(std::get<std::int64_t>(loaded[1].properties.at(WaypointOrderKey)) == 4,
                        "Waypoint order must round-trip.");
    }

    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    return passed ? 0 : 1;
}
