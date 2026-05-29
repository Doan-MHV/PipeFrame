#ifndef PIPEFRAME_PROJECTRUNTIME_H
#define PIPEFRAME_PROJECTRUNTIME_H

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "Fields/FieldGrid.h"

struct SDL_FRect;
struct SDL_Renderer;
class AssetRegistry;
class PrefabRegistry;
class ProjectConfig;
class Registry;
class TileMap;

struct ProjectRuntimeContext
{
    Registry& registry;
    TileMap& tileMap;
    const PrefabRegistry& prefabRegistry;
    const ProjectConfig& projectConfig;
    double deltaTime = 0.0;
    int elapsedTime = 0;
};

class ProjectSystem
{
public:
    virtual ~ProjectSystem() = default;
    virtual void OnWorldLoaded(Registry& registry) { (void)registry; }
    virtual void Update(ProjectRuntimeContext& context) = 0;
};

template <typename TSimulation>
class SimulationSystem
{
public:
    virtual ~SimulationSystem() = default;
    virtual void OnWorldLoaded(TSimulation& simulation, Registry& registry)
    {
        (void)simulation;
        (void)registry;
    }
    virtual void Update(TSimulation& simulation, ProjectRuntimeContext& context) = 0;
};

template <typename TSimulation>
class SimulationRenderPass
{
public:
    virtual ~SimulationRenderPass() = default;
    virtual void Render(
        const TSimulation& simulation,
        SDL_Renderer* renderer,
        AssetRegistry& assetRegistry,
        const SDL_FRect& camera
    ) = 0;
};

class ProjectSimulation
{
public:
    virtual ~ProjectSimulation() = default;
    virtual void Reset() = 0;
    virtual void Update(ProjectRuntimeContext& context) = 0;
    virtual void Render(
        SDL_Renderer* renderer,
        AssetRegistry& assetRegistry,
        const SDL_FRect& camera
    )
    {
        (void)renderer;
        (void)assetRegistry;
        (void)camera;
    }
    virtual const std::unordered_map<std::string, FieldGrid>& GetFieldGrids() const
    {
        static const std::unordered_map<std::string, FieldGrid> emptyFieldGrids;
        return emptyFieldGrids;
    }
};

struct ProjectObjectProperty
{
    std::string name;
    std::string value;
};

struct ProjectObjectInspector
{
    int id = -1;
    std::string typeName;
    std::string displayName;
    std::vector<ProjectObjectProperty> properties;
};

#endif // PIPEFRAME_PROJECTRUNTIME_H
