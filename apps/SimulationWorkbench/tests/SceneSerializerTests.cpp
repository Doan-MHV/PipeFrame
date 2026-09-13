#include "../Editor/SceneDocument.h"
#include "../Editor/SceneSerializer.h"

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <string>

namespace {

bool Check(
    const bool condition,
    const std::string &message) {

    if (!condition) {
        std::cerr
            << "FAILED: "
            << message
            << '\n';

        return false;
    }

    return true;
}

} // namespace

int main() {
    using namespace pipeframe::editor;

    bool passed = true;

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        "pipeframe_generic_scene_test.pfscene";

    SceneDocument document;

    PropertyMap properties;

    properties["agentCount"] =
        std::int64_t{10'000};

    properties["speed"] = 80.0;

    properties["enabled"] = true;

    properties["label"] =
        std::string{"Test population"};

    properties["spawnSize"] =
        pipeframe::Vector2f{1200.0f, 800.0f};

    const SceneObjectId objectId =
        document.CreateObject(
            "DEMO POPULATION",
            "basic.population",
            {{25.0f, 50.0f}, 15.0f, {2.0f, 0.5f}},
            std::move(properties));

    const SceneObjectId childId = document.CreateObject(
        "CHILD", "test.child", {{10.0f, 0.0f}, 30.0f, {0.5f, 2.0f}});
    passed &= Check(document.SetParent(childId, objectId), "Child should accept a valid parent.");
    passed &= Check(document.SetSettings({"cm", "degrees", "right-handed-y-up"}),
                    "Project units should be authorable.");
    passed &= Check(document.SetLayer(childId, "Sensors") && document.SetTags(childId,{"robot","sensor"}),
                    "Layer and tags should be authorable.");
    passed &= Check(document.SetVisible(childId,false) && document.SetLocked(childId,true),
                    "Visibility and locking should be authorable.");
    SceneComponentData sensor{"test.sensor",2,{{"target",SceneObjectReference{objectId}},
                                              {"tint",Color{1,2,3,4}},
                                              {"texture",AssetReference{"sensor.texture"}},
                                              {"enabled",true},
                                              {"samples",std::int64_t{16}},
                                              {"range",20.0},
                                              {"label",std::string{"Front sensor"}},
                                              {"offset",pipeframe::Vector2f{2.0f,3.0f}},
                                              {"mode",std::string{"Sweep"}}},true,false};
    const PropertyMap expectedSensorProperties=sensor.properties;
    passed &= Check(document.AddComponent(childId,std::move(sensor)),"Project component should attach.");
    passed &= Check(document.AddConnection({42, SceneConnectionKind::Signal,
                                            {objectId, "controller.out"}, {childId, "sensor.in"}}),
                    "Stable endpoint connection should attach.");
    const SceneTransform world=document.GetWorldTransform(childId);
    passed &= Check(world.position!=pipeframe::Vector2f{10.0f,0.0f} && world.scale==pipeframe::Vector2f{1.0f,1.0f},
                    "Nested transform should inherit parent rotation and scale.");
    passed &= Check(document.Validate().empty(),"Authored hierarchy should validate.");

    passed &= Check(
        objectId != 0,
        "Generic object should be created.");

    std::string errorMessage;

    passed &= Check(
        SceneSerializer::Save(
            document,
            path,
            &errorMessage),
        "Scene should save: " + errorMessage);

    const std::optional<SceneDocument> loaded =
        SceneSerializer::Load(
            path,
            &errorMessage);

    passed &= Check(
        loaded.has_value(),
        "Scene should load: " + errorMessage);

    if (loaded.has_value()) {
        passed &= Check(
            !loaded->IsDirty(),
            "Loaded document should be clean.");
        passed &= Check(loaded->GetSettings().lengthUnit == "cm" &&
                            loaded->GetConnections() == document.GetConnections(),
                        "Units, coordinates, and stable endpoint connections should survive.");

        const SceneObjectData *object =
            loaded->FindObject(objectId);

        passed &= Check(
            object != nullptr,
            "Loaded object should exist.");

        if (object != nullptr) {
            passed &= Check(
                object->typeId ==
                    "basic.population",
                "Project-owned type ID should survive.");

            passed &= Check(
                object->transform.position ==
                    pipeframe::Vector2f{25.0f, 50.0f},
                "Transform should survive.");

            passed &= Check(
                object->transform.scale ==
                    pipeframe::Vector2f{2.0f, 0.5f},
                "Transform scale should survive.");

            passed &= Check(
                object->properties.size() == 5,
                "All generic properties should survive.");

            const PropertyValue *agentCount =
                loaded->FindProperty(
                    objectId,
                    "agentCount");

            passed &= Check(
                agentCount != nullptr &&
                    std::get<std::int64_t>(
                        *agentCount) == 10'000,
                "Integer property should survive.");

            const SceneObjectData *child=loaded->FindObject(childId);
            passed &= Check(child && child->parentId==objectId && child->layer=="Sensors" &&
                            child->tags==std::vector<std::string>({"robot","sensor"}) && !child->visible && child->locked,
                            "Hierarchy metadata should survive.");
            if(child){
                const auto component=std::ranges::find_if(child->components,[](const auto &item){return item.typeId=="test.sensor";});
                passed &= Check(component!=child->components.end() && component->schemaVersion==2 &&
                                std::get<SceneObjectReference>(component->properties.at("target")).objectId==objectId &&
                                component->properties==expectedSensorProperties,
                                "Every Inspector property storage kind should round-trip.");
            }
        }
    }

    std::error_code removeError;
    std::filesystem::remove(path, removeError);

    if (!passed) {
        return 1;
    }

    std::cout
        << "All generic scene serializer tests passed.\n";

    return 0;
}
