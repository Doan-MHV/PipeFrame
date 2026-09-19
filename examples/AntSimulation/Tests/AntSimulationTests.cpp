#include "Runtime/AntRegistration.h"
#include "Runtime/AntSimulationRuntime.h"
#include "Runtime/AntTypeIds.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include <PipeFrame/Environment/TilemapSerializer.h>

namespace {

struct ExtensionComponent {
    double value{3};
    static auto Schema() {
        return pipeframe::ComponentSchema<ExtensionComponent>("test.extension", "Extension")
            .Editable(
                {.key = "value", .displayName = "Value", .kind = pipeframe::PropertyKind::Number, .defaultValue = 3.0},
                &ExtensionComponent::value);
    }
};
using ExtensionEntity = pipeframe::ComponentEntity<pipeframe::Transform2DComponent, ExtensionComponent>;
void Require(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';

        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace ant_simulation;

    AntSimulationRuntime runtime;
    std::string errorMessage;

    Require(runtime.Load({}, errorMessage), "Ant runtime should load without renderer assets in tests.");

    Require(errorMessage.empty(), "Successful load should not report an error.");

    const auto registered = AntEntityTypes().Describe(true);
    Require(registered.size() == 6, "Registry includes Ant, Colony, Food, Settings and Beacon");
    Require(std::ranges::any_of(registered, [](const auto &t) { return t.typeId == "ant.agent"; }),
            "Runtime-spawned Ant is registered");
    Require(AntEntityTypes().Describe().size() == 5, "Runtime Ant stays out of editor creation menu");
    const auto types = runtime.GetSceneObjectTypes();

    Require(types.size() == 5, "Runtime should expose colony, food, settings and beacon types.");
    Require(types[3].typeId == "ant.signal-beacon", "Fourth type is the project beacon entity.");

    Require(types[0].typeId == ColonyTypeId, "First type should be a colony.");

    Require(types[1].typeId == FoodSourceTypeId, "Second type should be food.");

    Require(types[2].typeId == SimulationSettingsTypeId, "Third type should be simulation settings.");

    Require(runtime.GetSceneComponentTypes().size() == 4,
            "Every Ant domain object type must expose a component schema.");

    const auto allComponentTypes = runtime.GetAllSceneComponentTypes();
    const auto findComponent = [&](const std::string_view id) {
        return std::ranges::find(allComponentTypes, id, &pipeframe::SceneComponentTypeDescriptor::typeId);
    };
    Require(findComponent(pipeframe::Transform2DComponentTypeId) != allComponentTypes.end(),
            "PipeFrame Transform must be available to Ant objects.");
    Require(findComponent("pipeframe.physics-body2d") != allComponentTypes.end(),
            "PipeFrame Physics Body 2D must be available to Ant objects.");
    const auto colonySchema = findComponent(ColonyTypeId);
    Require(colonySchema != allComponentTypes.end() && colonySchema->properties.size() == 7,
            "Colony metadata must expose all authored properties.");
    Require(colonySchema->properties[0].minimum == 0.0 && colonySchema->properties[0].maximum == 1'000'000.0,
            "Editor metadata must retain validation ranges.");

    pipeframe::SceneObjectData colony = runtime.CreateDefaultObject(ColonyTypeId);

    Require(
        std::ranges::any_of(colony.components, [](const auto &component) { return component.typeId == ColonyTypeId; }),
        "Colony settings must be stored in a project component.");

    colony.id = 1;

    colony.transform.position = {
        8.0f,
        8.0f,
    };

    colony.properties[InitialPopulationKey] = std::int64_t{2};

    colony.properties[SpawnRadiusKey] = 2.0;

    colony.properties[MovementSpeedKey] = 2.0;

    colony.properties[RandomSeedKey] = std::int64_t{42};

    pipeframe::SceneObjectData food = runtime.CreateDefaultObject(FoodSourceTypeId);

    food.id = 2;

    food.transform.position = {
        20.5f,
        20.5f,
    };

    food.properties[FoodAmountKey] = std::int64_t{25};

    food.properties[FoodRadiusKey] = 1.5;

    const auto fixtureRoot = std::filesystem::temp_directory_path() / "pipeframe-ant-runtime-map-test";
    std::filesystem::remove_all(fixtureRoot);
    std::filesystem::create_directories(fixtureRoot);
    const auto mapPath = fixtureRoot / "Runtime.pftilemap";
    pipeframe::Tilemap2D map(32, 24);
    map.DefineTile({1, {45, 42, 38, 255}, true});
    map.AddLayer("Terrain");
    map.SetTile(0, {12, 12}, 1);
    map.AddDataLayer("ant.food-density", 0, 10000);
    map.SetData("ant.food-density", {13, 12}, 7);
    {
        std::ofstream out(mapPath);
        Require(pipeframe::TilemapSerializer::Save(map, out), "Save runtime tilemap fixture");
    }
    pipeframe::assets::AssetDatabase fixtureDatabase;
    Require(fixtureDatabase.Open(fixtureRoot, &errorMessage), "Open fixture assets");
    auto fixtureMap = fixtureDatabase.ImportNow({mapPath}, &errorMessage);
    Require(bool(fixtureMap), "Import fixture tilemap");
    pipeframe::ServiceRegistry fixtureServices;
    fixtureServices.Provide(fixtureDatabase);
    pipeframe::ProjectRuntimeContext fixtureContext;
    fixtureContext.services = &fixtureServices;
    runtime.Unload();
    Require(runtime.Load(fixtureContext, errorMessage), "Load runtime with typed map services");

    pipeframe::SceneObjectData settings = runtime.CreateDefaultObject(SimulationSettingsTypeId);

    settings.id = 3;

    settings.properties[ShowGridKey] = false;

    settings.properties[ShowMarkersKey] = true;

    settings.properties[ShowShadowsKey] = false;

    settings.properties[ShowTargetsKey] = true;

    settings.properties[ShowPhysicsDebugKey] = true;

    settings.properties[ShowAntsKey] = false;

    settings.properties[DynamicAntColorsKey] = true;

    settings.properties[MarkerIntensityKey] = std::int64_t{15};

    settings.properties[MapAssetKey] = pipeframe::AssetReference{*fixtureMap};

    settings.properties[UpdateWorkerCountKey] = std::int64_t{4};

    std::vector<pipeframe::SceneObjectData> objects{
        colony,
        food,
        settings,
    };

    runtime.SynchronizeScene(objects);

    Require(runtime.GetSimulationWorld() != nullptr, "Map synchronization should build the simulation world.");

    Require(runtime.GetSimulationWorld()->GetEnvironment().GetWidth() == 32 &&
                runtime.GetSimulationWorld()->GetEnvironment().GetHeight() == 24,
            "Map dimensions should define the world size.");

    Require(runtime.GetSimulationWorld()->GetConfiguration().antUpdateWorkerCount == 4,
            "Simulation settings should expose the deterministic/parallel worker count.");

    Require(runtime.GetSimulationWorld()->GetEnvironment().TryGetCell(12, 12)->wall,
            "Red map pixels should create walls.");

    Require(runtime.GetSimulationWorld()->GetEnvironment().TryGetCell(13, 12)->foodQuantity > 0,
            "Green map pixels should create food.");

    const auto &renderOptions = runtime.GetRenderOptions();

    Require(!renderOptions.showGrid, "Settings should disable the grid.");

    Require(renderOptions.showMarkers, "Settings should enable markers.");

    Require(!renderOptions.showShadows, "Settings should disable shadows.");

    Require(renderOptions.showTargets, "Settings should enable targets.");

    Require(renderOptions.showPhysicsDebug, "Settings should enable physics debugging.");

    Require(!renderOptions.showAnts, "Settings should disable ant rendering.");

    Require(renderOptions.dynamicAntColors, "Settings should enable dynamic ant colors.");

    Require(renderOptions.markerIntensity == 15, "Settings should load marker intensity.");

    Require(runtime.GetSimulationWorld()->GetStatistics().colonyCount == 1, "Runtime should create authored colony.");

    const auto initialFood = runtime.GetRemainingFood(food.id);

    Require(initialFood.has_value() && *initialFood > 0, "Authored food patch should contain food.");

    Require(runtime.GetDeliveredFood(colony.id) == std::optional<std::int64_t>{0},
            "Colony should begin with no delivered food.");

    const auto colonyHit = runtime.HitTest({
        8.0f,
        8.0f,
    });

    Require(colonyHit.has_value() && *colonyHit == colony.id, "Colony should be selectable.");

    const auto foodHit = runtime.HitTest({
        20.5f,
        20.5f,
    });

    Require(foodHit.has_value() && *foodHit == food.id, "Food should be selectable.");

    runtime.SetSelectedObject(colony.id);

    runtime.Start();

    runtime.FixedUpdate(1.0f / 60.0f);

    Require(runtime.GetSimulationWorld()->GetStatistics().antCount == 1, "First tick should spawn one ant.");

    runtime.FixedUpdate(1.0f / 60.0f);

    Require(runtime.GetSimulationWorld()->GetStatistics().antCount == 2, "Second tick should spawn second funded ant.");

    Require(runtime.GetSimulationWorld()->GetStatistics().physicsBodyCount == 2,
            "Spawned ants should have physics bodies.");

    runtime.Stop();

    runtime.Reset();

    Require(runtime.GetSimulationWorld()->GetStatistics().tick == 0, "Reset should clear simulation tick.");

    Require(runtime.GetSimulationWorld()->GetStatistics().antCount == 0,
            "Reset should restore authored pre-play state.");

    Require(runtime.GetRemainingFood(food.id) == initialFood, "Reset should restore food patch.");

    Require(runtime.GetDeliveredFood(colony.id) == std::optional<std::int64_t>{0},
            "Reset should clear delivered food.");

    const auto statistics = runtime.GetStatistics();

    Require(statistics.available, "Loaded runtime statistics should be available.");

    runtime.RegisterComponent<ExtensionComponent>();
    runtime.RegisterEntity<ExtensionEntity>({"test.extension-entity", "Extension", {}, {"test.extension"}});
    Require(std::ranges::any_of(runtime.GetSceneObjectTypes(),
                                [](const auto &type) { return type.typeId == "test.extension-entity"; }),
            "New registered extension appears without runtime branches");
    auto extension = runtime.CreateDefaultObject("test.extension-entity");
    extension.id = 100;
    std::vector<pipeframe::SceneObjectData> extensionScene{extension};
    runtime.SynchronizeScene(extensionScene);
    Require(runtime.ResolveSceneObject(100).GetComponent<ExtensionComponent>()->value == 3,
            "Ant runtime instantiates arbitrary registered composition");
    runtime.Unload();

    Require(runtime.GetSimulationWorld() == nullptr, "Unload should release simulation world.");

    // Typed maps use the same host asset service as generated projects.
    const auto assetRoot = std::filesystem::temp_directory_path() / "pipeframe-ant-typed-map-test";
    std::filesystem::remove_all(assetRoot);
    std::filesystem::create_directories(assetRoot);
    pipeframe::assets::AssetDatabase database;
    Require(database.Open(assetRoot, &errorMessage), "Open Ant asset database");
    pipeframe::Tilemap2D painted(32, 24);
    painted.DefineTile({1, {90, 100, 110, 255}, true});
    painted.AddLayer("Terrain");
    painted.SetTile(0, {12, 12}, 1);
    const auto source = assetRoot / "painted.pftilemap";
    {
        std::ofstream out(source);
        Require(pipeframe::TilemapSerializer::Save(painted, out), "Save authored map");
    }
    const auto asset = database.ImportNow({source}, &errorMessage);
    Require(asset.has_value(), "Import Ant map");
    pipeframe::ServiceRegistry services;
    services.Provide(database);
    pipeframe::ProjectRuntimeContext context;
    context.services = &services;
    Require(runtime.Load(context, errorMessage), "Load Ant with host asset services");
    settings = runtime.CreateDefaultObject(SimulationSettingsTypeId);
    settings.id = 3;
    settings.properties[MapAssetKey] = pipeframe::AssetReference{*asset};
    objects = {colony, food, settings};
    runtime.SynchronizeScene(objects);
    Require(runtime.GetSimulationWorld() && runtime.GetSimulationWorld()->GetEnvironment().TryGetCell(12, 12)->wall,
            "Typed map supplies Ant walls");
    painted.SetTile(0, {12, 12}, 0);
    painted.SetTile(0, {14, 12}, 1);
    {
        std::ofstream out(assetRoot / database.Find(*asset)->sourcePath);
        pipeframe::TilemapSerializer::Save(painted, out);
    }
    Require(database.Reimport(*asset, &errorMessage), "Reimport painted Ant map");
    runtime.Reset();
    Require(!runtime.GetSimulationWorld()->GetEnvironment().TryGetCell(12, 12)->wall &&
                runtime.GetSimulationWorld()->GetEnvironment().TryGetCell(14, 12)->wall,
            "Reset resolves the newly saved asset revision");
    runtime.Unload();
    std::filesystem::remove_all(assetRoot);

    std::cout << "All ant simulation tests passed.\n";

    std::filesystem::remove_all(fixtureRoot);

    return 0;
}
