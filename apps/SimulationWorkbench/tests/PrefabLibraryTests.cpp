#include "../Editor/PrefabLibrary.h"
#include "../Editor/SceneHistory.h"
#include "../Editor/SceneSerializer.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {

void Require(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

const pipeframe::editor::SceneObjectData *FindBySource(
    const pipeframe::editor::SceneDocument &scene, const std::string &prefabId,
    const pipeframe::editor::SceneObjectId rootId,
    const pipeframe::editor::SceneObjectId sourceId) {
    for (const auto &object : scene.GetObjects())
        for (const auto &link : object.prefabLinks)
            if (link.prefabId == prefabId && link.instanceRootId == rootId &&
                link.sourceObjectId == sourceId) return &object;
    return nullptr;
}

} // namespace

int main() {
    using namespace pipeframe::editor;
    PrefabLibrary library;
    std::string error;

    SceneDocument wheelScene;
    const auto wheel = wheelScene.CreateObject("Wheel", "robot.wheel");
    SceneObjectData sensorObject;
    sensorObject.name = "Encoder";
    sensorObject.typeId = "robot.encoder";
    sensorObject.parentId = wheel;
    sensorObject.properties.emplace("ticks", std::int64_t{20});
    const auto encoder = wheelScene.CreateObject(std::move(sensorObject));
    Require(wheelScene.AddConnection({1, SceneConnectionKind::Signal,
                                      {wheel, "shaft"}, {encoder, "shaft"}}),
            "Nested source connection should be created.");
    auto wheelPrefab = library.Create("wheel", wheelScene, wheel, &error);
    Require(wheelPrefab.has_value(), "Wheel prefab should be created.");
    Require(library.AdoptInstance(*wheelPrefab, wheelScene, wheel, &error) &&
                wheelScene.FindObject(wheel)->prefabLinks.size() == 1,
            "Creating a prefab should be able to adopt the authored hierarchy as its first instance.");

    SceneDocument assembly;
    const auto chassis = assembly.CreateObject("Chassis", "robot.chassis");
    SceneTransform wheelPlacement;
    wheelPlacement.position = {10.0f, 5.0f};
    auto nestedWheel = library.Instantiate(*wheelPrefab, assembly, chassis, wheelPlacement, &error);
    Require(nestedWheel.has_value(), "Nested wheel instance should be created.");
    auto robotPrefab = library.Create("robot", assembly, chassis, &error);
    Require(robotPrefab.has_value(), "Assembly prefab should be created.");

    const auto folder = std::filesystem::temp_directory_path() / "pipeframe-prefab-tests";
    const auto prefabPath = folder / "robot.pfprefab";
    Require(library.Save(*robotPrefab, prefabPath, &error), "Prefab should save.");
    auto loadedPrefab = library.Load(prefabPath, &error);
    Require(loadedPrefab.has_value() && loadedPrefab->source.GetObjects().size() == 3,
            "Prefab source and hierarchy should load.");

    SceneDocument world;
    SceneTransform placement;
    placement.position = {100.0f, 80.0f};
    auto first = library.Instantiate(*loadedPrefab, world, 0, placement, &error);
    auto second = library.Instantiate(*loadedPrefab, world, 0, {}, &error);
    Require(first.has_value() && second.has_value(), "Multiple prefab instances should be created.");
    const auto *nestedObject = FindBySource(world, "robot", first->rootObjectId, 2);
    Require(nestedObject && nestedObject->prefabLinks.size() == 2,
            "Nested object should preserve inner and outer prefab identity.");

    const auto nestedId = nestedObject->id;
    SceneHistory history;
    history.Record(world);
    auto moved = nestedObject->transform;
    moved.position.x += 7.0f;
    Require(world.SetTransform(nestedId, moved), "Instance override should be editable.");
    Require(!library.GetOverrides(*loadedPrefab, world, first->rootObjectId).empty(),
            "Prefab overrides should be reported.");
    Require(history.Undo(world), "Prefab instance edit should participate in undo.");
    Require(history.Redo(world), "Prefab instance edit should participate in redo.");

    const auto scenePath = folder / "World.pfscene";
    Require(SceneSerializer::Save(world, scenePath, &error), "Scene with prefab links should save.");
    auto loadedWorld = SceneSerializer::Load(scenePath, &error);
    Require(loadedWorld.has_value() &&
                FindBySource(*loadedWorld, "robot", first->rootObjectId, 2)->prefabLinks.size() == 2,
            "Nested prefab links should survive scene serialization.");

    Require(library.RevertInstance(*loadedPrefab, world, first->rootObjectId, &error),
            "Prefab instance should revert.");
    Require(library.GetOverrides(*loadedPrefab, world, first->rootObjectId).empty(),
            "Revert should clear overrides while preserving instance IDs.");
    Require(FindBySource(world, "robot", first->rootObjectId, 2)->id == nestedId,
            "Revert should preserve stable scene object identity.");

    const auto *secondNested = FindBySource(world, "robot", second->rootObjectId, 2);
    auto changed = secondNested->transform;
    changed.position.y += 9.0f;
    Require(world.SetTransform(secondNested->id, changed), "Second instance should accept an override.");
    Require(library.ApplyInstance(*loadedPrefab, world, second->rootObjectId, &error),
            "Instance overrides should apply to the prefab source.");
    Require(loadedPrefab->revision == 2, "Apply should advance the prefab revision.");
    Require(!library.GetConflicts(*loadedPrefab, world, first->rootObjectId).empty(),
            "Older instances should report a revision conflict.");
    Require(library.RevertInstance(*loadedPrefab, world, first->rootObjectId, &error),
            "An older instance should update from the new prefab revision.");
    Require(FindBySource(world, "robot", first->rootObjectId, 2)->transform.position.y == changed.position.y,
            "Applied source change should propagate on revert/update.");

    auto variantSource = loadedPrefab->source;
    const auto sourceChild = variantSource.GetObjects().at(1).id;
    Require(variantSource.RenameObject(sourceChild, "Variant Wheel"),
            "Variant source should be customizable.");
    auto variant = library.CreateVariant("robot.fast", *loadedPrefab, std::move(variantSource), &error);
    Require(variant.has_value() && variant->basePrefabId == "robot",
            "Variant should retain its base prefab identity.");

    Require(library.UnpackInstance(world, first->rootObjectId, &error),
            "Outer prefab should unpack.");
    const auto *unpackedNested = world.FindObject(nestedId);
    Require(unpackedNested && unpackedNested->prefabLinks.size() == 1 &&
                unpackedNested->prefabLinks.front().prefabId == "wheel",
            "Unpacking an assembly should preserve its nested prefab instance.");

    std::error_code cleanupError;
    std::filesystem::remove_all(folder, cleanupError);
    std::cout << "All prefab library tests passed.\n";
    return 0;
}
