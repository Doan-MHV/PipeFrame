#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "../Editor/SceneDocument.h"
#include "../Editor/SceneSerializer.h"

namespace {

using namespace pipeframe::editor;

class TemporarySceneFile {
  public:
    explicit TemporarySceneFile(std::string name) {
        const auto uniqueValue = std::chrono::steady_clock::now().time_since_epoch().count();

        path = std::filesystem::temp_directory_path() / (std::move(name) + std::to_string(uniqueValue) + ".pfscene");
    }

    ~TemporarySceneFile() {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    const std::filesystem::path &GetPath() const { return path; }

  private:
    std::filesystem::path path;
};

bool Check(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    return true;
}

bool TestPopulationRoundTrip() {
    TemporarySceneFile file{"pipeframe_population_"};

    SceneDocument document;

    document.CreateObject("DEMO AGENT", SceneObjectType::DemoAgent,
                          {
                              {10.0f, 20.0f},
                              45.0f,
                          });

    AgentPopulationSettings settings;

    settings.agentCount = 1'000'000;
    settings.spawnAreaSize = {4000.0f, 2500.0f};
    settings.randomSeed = 12345;

    const SceneObjectId populationId = document.CreatePopulation("ANT COLONY",
                                                                 {
                                                                     {100.0f, -200.0f},
                                                                     15.0f,
                                                                 },
                                                                 settings);

    std::string errorMessage;

    if (!SceneSerializer::Save(document, file.GetPath(), &errorMessage)) {

        std::cerr << "FAILED: " << errorMessage << '\n';
        return false;
    }

    const std::optional<SceneDocument> loadedDocument = SceneSerializer::Load(file.GetPath(), &errorMessage);

    if (!loadedDocument.has_value()) {
        std::cerr << "FAILED: " << errorMessage << '\n';
        return false;
    }

    bool passed = true;

    passed &= Check(loadedDocument->GetObjects().size() == 2, "Expected two loaded scene objects.");

    passed &= Check(!loadedDocument->IsDirty(), "Loaded document should be clean.");

    const SceneObjectData *population = loadedDocument->FindObject(populationId);

    passed &= Check(population != nullptr, "Population ID was not restored.");

    if (population == nullptr) {
        return false;
    }

    passed &= Check(population->type == SceneObjectType::AgentPopulation, "Loaded object has the wrong type.");

    passed &= Check(population->name == "ANT COLONY", "Population name was not restored.");

    passed &=
        Check(population->transform.position == sf::Vector2f{100.0f, -200.0f}, "Population position was not restored.");

    passed &= Check(population->transform.rotation == 15.0f, "Population rotation was not restored.");

    passed &= Check(population->population.has_value(), "Population settings are missing.");

    if (!population->population.has_value()) {
        return false;
    }

    const AgentPopulationSettings &loadedSettings = *population->population;

    passed &= Check(loadedSettings.agentCount == 1'000'000, "Agent count was not restored.");

    passed &=
        Check(loadedSettings.spawnAreaSize == sf::Vector2f{4000.0f, 2500.0f}, "Spawn-area size was not restored.");

    passed &= Check(loadedSettings.randomSeed == 12345, "Random seed was not restored.");

    return passed;
}

bool TestPopulationEditing() {
    SceneDocument document;

    const SceneObjectId populationId = document.CreatePopulation("TEST POPULATION", {}, {});

    document.MarkClean();

    AgentPopulationSettings settings;
    settings.agentCount = 0;
    settings.spawnAreaSize = {0.0f, -50.0f};
    settings.randomSeed = 42;

    bool passed = true;

    passed &= Check(document.SetPopulationSettings(populationId, settings), "Population settings should be changed.");

    const SceneObjectData *population = document.FindObject(populationId);

    passed &=
        Check(population != nullptr && population->population.has_value(), "Edited population settings are missing.");

    if (population == nullptr || !population->population.has_value()) {
        return false;
    }

    const AgentPopulationSettings &edited = *population->population;

    passed &= Check(edited.agentCount == 1, "Agent count should be clamped to one.");

    passed &= Check(edited.spawnAreaSize == sf::Vector2f{1.0f, 1.0f}, "Spawn area should be clamped to one.");

    passed &= Check(edited.randomSeed == 42, "Random seed was not updated.");

    passed &= Check(document.IsDirty(), "Editing a population should dirty the document.");

    document.MarkClean();

    passed &= Check(!document.SetPopulationSettings(populationId, settings),
                    "Applying identical settings should do nothing.");

    passed &= Check(!document.IsDirty(), "An unchanged population should remain clean.");

    return passed;
}

bool TestVersionOneCompatibility() {
    TemporarySceneFile file{"pipeframe_version_one_"};

    {
        std::ofstream output(file.GetPath());

        output << "PIPEFRAME_SCENE 1\n";
        output << "1\n";
        output << "42 0 12.5 -3.25 90 \"LEGACY AGENT\"\n";
    }

    std::string errorMessage;

    const std::optional<SceneDocument> loadedDocument = SceneSerializer::Load(file.GetPath(), &errorMessage);

    if (!loadedDocument.has_value()) {
        std::cerr << "FAILED: " << errorMessage << '\n';
        return false;
    }

    const SceneObjectData *object = loadedDocument->FindObject(42);

    bool passed = true;

    passed &= Check(object != nullptr, "Version-1 object was not restored.");

    if (object == nullptr) {
        return false;
    }

    passed &= Check(object->type == SceneObjectType::DemoAgent, "Version-1 object has the wrong type.");

    passed &= Check(object->name == "LEGACY AGENT", "Version-1 name was not restored.");

    passed &= Check(!object->population.has_value(), "Version-1 object unexpectedly has population data.");

    return passed;
}

} // namespace

int main() {
    bool passed = true;

    passed &= TestPopulationRoundTrip();
    passed &= TestPopulationEditing();
    passed &= TestVersionOneCompatibility();

    if (!passed) {
        return 1;
    }

    std::cout << "All scene serializer tests passed.\n";
    return 0;
}
