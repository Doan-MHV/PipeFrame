#ifndef PIPEFRAME_PROJECTRUNTIME_H
#define PIPEFRAME_PROJECTRUNTIME_H

#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "ECS/Registry.h"
#include "Fields/FieldGrid.h"
#include "Game/LevelLoadRequests.h"

struct SDL_FRect;
struct SDL_Renderer;
class AssetRegistry;
class ComponentRegistry;
class PrefabRegistry;
class ProjectConfig;
class Registry;
class TileMap;

struct ProjectRuntimeContext {
    Registry &registry;
    const ComponentRegistry &componentRegistry;
    TileMap &tileMap;
    const PrefabRegistry &prefabRegistry;
    const ProjectConfig &projectConfig;
    LevelLoadRequests &levels;
    double deltaTime = 0.0;
    int elapsedTime = 0;
};

template <typename... TComponents, typename TCallback>
void ForEachProjectEntity(Registry &registry, TCallback &&callback) {
    for (Entity entity : registry.GetAllEntities()) {
        if (!(entity.HasComponent<TComponents>() && ...)) {
            continue;
        }

        if constexpr (std::is_invocable_v<TCallback &, Entity, TComponents &...>) {
            callback(entity, entity.GetComponent<TComponents>()...);
        } else {
            callback(entity.GetComponent<TComponents>()...);
        }
    }
}

class ProjectSystem {
  public:
    virtual ~ProjectSystem() = default;
    virtual void Loaded(ProjectRuntimeContext &context) { (void)context; }
    virtual void Start(ProjectRuntimeContext &context) { (void)context; }
    virtual void Update(ProjectRuntimeContext &context) { (void)context; }
    virtual void Stop(ProjectRuntimeContext &context) { (void)context; }
    virtual void Unloaded(ProjectRuntimeContext &context) { (void)context; }
};

template <typename TSimulation> class SimulationSystem {
  public:
    virtual ~SimulationSystem() = default;
    virtual void Loaded(TSimulation &simulation, ProjectRuntimeContext &context) {
        (void)simulation;
        (void)context;
    }
    virtual void Start(TSimulation &simulation, ProjectRuntimeContext &context) {
        (void)simulation;
        (void)context;
    }
    virtual void Update(TSimulation &simulation, ProjectRuntimeContext &context) {
        (void)simulation;
        (void)context;
    }
    virtual void Stop(TSimulation &simulation, ProjectRuntimeContext &context) {
        (void)simulation;
        (void)context;
    }
    virtual void Unloaded(TSimulation &simulation, ProjectRuntimeContext &context) {
        (void)simulation;
        (void)context;
    }
};

template <typename TSimulation> class SimulationRenderPass {
  public:
    virtual ~SimulationRenderPass() = default;
    virtual void Render(const TSimulation &simulation, SDL_Renderer *renderer, AssetRegistry &assetRegistry,
                        const SDL_FRect &camera) = 0;
};

class ProjectSimulation {
  public:
    virtual ~ProjectSimulation() = default;
    virtual void Loaded(ProjectRuntimeContext &context) { (void)context; }
    virtual void Start(ProjectRuntimeContext &context) { (void)context; }
    virtual void Update(ProjectRuntimeContext &context) { (void)context; }
    virtual void Stop(ProjectRuntimeContext &context) { (void)context; }
    virtual void Unloaded(ProjectRuntimeContext &context) { (void)context; }
    virtual void Render(SDL_Renderer *renderer, AssetRegistry &assetRegistry, const SDL_FRect &camera) {
        (void)renderer;
        (void)assetRegistry;
        (void)camera;
    }
    virtual const std::unordered_map<std::string, FieldGrid> &GetFieldGrids() const {
        static const std::unordered_map<std::string, FieldGrid> emptyFieldGrids;
        return emptyFieldGrids;
    }
};

struct ProjectObjectProperty {
    std::string name;
    std::string value;
};

struct ProjectObjectInspector {
    int id = -1;
    std::string typeName;
    std::string displayName;
    std::vector<ProjectObjectProperty> properties;
};

#endif // PIPEFRAME_PROJECTRUNTIME_H
