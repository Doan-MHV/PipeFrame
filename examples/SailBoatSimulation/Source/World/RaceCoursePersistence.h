#ifndef SAILBOAT_RACE_COURSE_PERSISTENCE_H
#define SAILBOAT_RACE_COURSE_PERSISTENCE_H

#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <PipeFrame/Project/ProjectTypes.h>

namespace sailboat_simulation {

class RaceCoursePersistence final {
public:
    static bool Save(const std::filesystem::path &path,
                     std::span<const pipeframe::SceneObjectData> objects,
                     std::string &errorMessage);
    static bool Load(const std::filesystem::path &path,
                     std::vector<pipeframe::SceneObjectData> &objects,
                     std::string &errorMessage);
};

} // namespace sailboat_simulation

#endif
