#ifndef PIPEFRAME_PROJECTMODULE_H
#define PIPEFRAME_PROJECTMODULE_H

#include <unordered_map>
#include <string>

#include <glm/glm.hpp>

#include "Simulation/ProjectRuntime.h"

struct SDL_FRect;
struct SDL_Renderer;
class AssetRegistry;
class ClassRegistry;
class ComponentRegistry;
class ProjectConfig;
class Registry;

class ProjectModule
{
public:
    virtual ~ProjectModule() = default;

    virtual std::string GetName() const = 0;
    virtual void RegisterComponents(ComponentRegistry& registry) { (void)registry; }
    virtual void RegisterEntityClasses(ClassRegistry& registry) { (void)registry; }
    virtual void RegisterEntitySystems(Registry& registry) = 0;
    virtual void Loaded(ProjectRuntimeContext& context) { (void)context; }
    virtual void Start(ProjectRuntimeContext& context) { (void)context; }
    virtual void Update(ProjectRuntimeContext& context) { (void)context; }
    virtual void Stop(ProjectRuntimeContext& context) { (void)context; }
    virtual void Unloaded(ProjectRuntimeContext& context) { (void)context; }
    virtual void RenderProjectSimulation(
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
    virtual int PickProjectObject(glm::vec2 worldPosition, float radius)
    {
        (void)worldPosition;
        (void)radius;
        return -1;
    }
    virtual void SetSelectedProjectObject(int id)
    {
        (void)id;
    }
    virtual ProjectObjectInspector GetSelectedProjectObjectInspector() const
    {
        return {};
    }
};

using CreateProjectModuleFn = ProjectModule* (*)();
using DestroyProjectModuleFn = void (*)(ProjectModule*);

#endif // PIPEFRAME_PROJECTMODULE_H
