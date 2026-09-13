#include "RaceCoursePersistence.h"

#include <fstream>
#include <iomanip>

#include "Components/SailBoatSimulationTypes.h"

namespace sailboat_simulation {
namespace {

constexpr const char *Magic = "PIPEFRAME_SAILBOAT_RACE";
constexpr unsigned Version = 1;

double Number(const pipeframe::SceneObjectData &object, const char *key, const double fallback) {
    const auto found = object.properties.find(key);
    if (found == object.properties.end()) {
        return fallback;
    }
    if (const auto *value = std::get_if<double>(&found->second)) {
        return *value;
    }
    if (const auto *value = std::get_if<std::int64_t>(&found->second)) {
        return static_cast<double>(*value);
    }
    return fallback;
}

bool IsRaceType(const std::string &type) {
    return type == RaceStartTypeId || type == FinishLineTypeId || type == WaypointTypeId;
}

} // namespace

bool RaceCoursePersistence::Save(const std::filesystem::path &path,
                                 const std::span<const pipeframe::SceneObjectData> objects,
                                 std::string &errorMessage) {
    errorMessage.clear();
    std::error_code filesystemError;
    std::filesystem::create_directories(path.parent_path(), filesystemError);
    if (filesystemError) {
        errorMessage = "Unable to create race directory: " + filesystemError.message();
        return false;
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        errorMessage = "Unable to save race: " + path.string();
        return false;
    }

    std::size_t count = 0;
    for (const auto &object : objects) {
        count += IsRaceType(object.typeId) ? 1u : 0u;
    }
    output << Magic << ' ' << Version << ' ' << count << '\n' << std::setprecision(9);
    for (const auto &object : objects) {
        if (!IsRaceType(object.typeId)) {
            continue;
        }
        output << std::quoted(object.typeId) << ' ' << std::quoted(object.name) << ' '
               << object.transform.position.x << ' ' << object.transform.position.y << ' '
               << object.transform.rotation << ' ' << Number(object, LineLengthKey, 80.0) << ' '
               << Number(object, WaypointRadiusKey, 10.0) << ' '
               << static_cast<std::int64_t>(Number(object, WaypointOrderKey, 1.0)) << '\n';
    }
    if (!output) {
        errorMessage = "Unable to finish writing race: " + path.string();
        return false;
    }
    return true;
}

bool RaceCoursePersistence::Load(const std::filesystem::path &path,
                                 std::vector<pipeframe::SceneObjectData> &objects,
                                 std::string &errorMessage) {
    errorMessage.clear();
    std::ifstream input(path);
    std::string magic;
    unsigned version = 0;
    std::size_t count = 0;
    if (!(input >> magic >> version >> count) || magic != Magic || version != Version) {
        errorMessage = "Invalid or unsupported race file: " + path.string();
        return false;
    }

    std::vector<pipeframe::SceneObjectData> loaded;
    loaded.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        pipeframe::SceneObjectData object;
        double length = 0.0;
        double radius = 0.0;
        std::int64_t order = 0;
        if (!(input >> std::quoted(object.typeId) >> std::quoted(object.name)
                    >> object.transform.position.x >> object.transform.position.y
                    >> object.transform.rotation >> length >> radius >> order) ||
            !IsRaceType(object.typeId)) {
            errorMessage = "Invalid race object in: " + path.string();
            return false;
        }
        object.properties.emplace(LineLengthKey, length);
        if (object.typeId == WaypointTypeId) {
            object.properties.emplace(WaypointRadiusKey, radius);
            object.properties.emplace(WaypointOrderKey, order);
        }
        loaded.push_back(std::move(object));
    }
    objects = std::move(loaded);
    return true;
}

} // namespace sailboat_simulation
