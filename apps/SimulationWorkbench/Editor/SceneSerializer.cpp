#include "SceneSerializer.h"

#include <fstream>
#include <iomanip>
#include <limits>
#include <utility>

namespace pipeframe::editor {

namespace {

constexpr unsigned int CurrentSceneFormatVersion = 2;
constexpr unsigned int OldestSupportedSceneFormatVersion = 1;

constexpr const char *SceneFileHeader = "PIPEFRAME_SCENE";

void SetError(std::string *errorMessage, std::string message) {
    if (errorMessage != nullptr) {
        *errorMessage = std::move(message);
    }
}

} // namespace

bool SceneSerializer::Save(const SceneDocument &document, const std::filesystem::path &path,
                           std::string *errorMessage) {

    std::ofstream output(path);

    if (!output.is_open()) {
        SetError(errorMessage, "Could not open scene file for writing: " + path.string());

        return false;
    }

    output << SceneFileHeader << ' ' << CurrentSceneFormatVersion << '\n';

    output << document.GetObjects().size() << '\n';

    output << std::setprecision(std::numeric_limits<float>::max_digits10);

    for (const SceneObjectData &object : document.GetObjects()) {
        output << object.id << ' ' << static_cast<int>(object.type) << ' ' << object.transform.position.x << ' '
               << object.transform.position.y << ' ' << object.transform.rotation << ' ' << std::quoted(object.name);

        if (object.type == SceneObjectType::AgentPopulation) {
            if (!object.population.has_value()) {
                SetError(errorMessage, "Population object is missing population settings.");

                return false;
            }

            const AgentPopulationSettings &settings = *object.population;

            output << ' ' << settings.agentCount << ' ' << settings.spawnAreaSize.x << ' ' << settings.spawnAreaSize.y
                   << ' ' << settings.randomSeed;
        } else if (object.type != SceneObjectType::DemoAgent) {
            SetError(errorMessage, "Scene contains an unsupported object type.");

            return false;
        }

        output << '\n';
    }

    if (!output.good()) {
        SetError(errorMessage, "Failed while writing scene file: " + path.string());

        return false;
    }

    return true;
}

std::optional<SceneDocument> SceneSerializer::Load(const std::filesystem::path &path, std::string *errorMessage) {

    std::ifstream input(path);

    if (!input.is_open()) {
        SetError(errorMessage, "Could not open scene file for reading: " + path.string());

        return std::nullopt;
    }

    std::string header;
    unsigned int version = 0;

    if (!(input >> header >> version)) {
        SetError(errorMessage, "Scene file header is missing or invalid.");

        return std::nullopt;
    }

    if (header != SceneFileHeader) {
        SetError(errorMessage, "This is not a PipeFrame scene file.");

        return std::nullopt;
    }

    if (version < OldestSupportedSceneFormatVersion || version > CurrentSceneFormatVersion) {

        SetError(errorMessage, "Unsupported scene file version.");

        return std::nullopt;
    }

    std::size_t objectCount = 0;

    if (!(input >> objectCount)) {
        SetError(errorMessage, "Scene object count is missing.");

        return std::nullopt;
    }

    SceneDocument loadedDocument;

    for (std::size_t index = 0; index < objectCount; ++index) {
        SceneObjectId objectId = 0;
        int objectTypeValue = 0;

        SceneTransform transform;
        std::string name;

        if (!(input >> objectId >> objectTypeValue >> transform.position.x >> transform.position.y >>
              transform.rotation >> std::quoted(name))) {

            SetError(errorMessage, "Scene object data is incomplete or invalid.");

            return std::nullopt;
        }

        SceneObjectData object;

        object.id = objectId;
        object.name = std::move(name);
        object.transform = transform;

        if (objectTypeValue == static_cast<int>(SceneObjectType::DemoAgent)) {

            object.type = SceneObjectType::DemoAgent;
        } else if (objectTypeValue == static_cast<int>(SceneObjectType::AgentPopulation)) {

            if (version < 2) {
                SetError(errorMessage, "Version-1 scenes cannot contain populations.");

                return std::nullopt;
            }

            AgentPopulationSettings settings;

            if (!(input >> settings.agentCount >> settings.spawnAreaSize.x >> settings.spawnAreaSize.y >>
                  settings.randomSeed)) {

                SetError(errorMessage, "Population settings are incomplete or invalid.");

                return std::nullopt;
            }

            if (settings.agentCount == 0 || settings.spawnAreaSize.x <= 0.0f || settings.spawnAreaSize.y <= 0.0f) {

                SetError(errorMessage, "Population settings contain invalid values.");

                return std::nullopt;
            }

            object.type = SceneObjectType::AgentPopulation;
            object.population = settings;
        } else {
            SetError(errorMessage, "Scene contains an unsupported object type.");

            return std::nullopt;
        }

        if (!loadedDocument.RestoreObject(std::move(object))) {
            SetError(errorMessage, "Scene contains duplicate object IDs.");

            return std::nullopt;
        }
    }

    loadedDocument.MarkClean();

    return loadedDocument;
}

} // namespace pipeframe::editor