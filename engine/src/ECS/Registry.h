

#ifndef PIPEFRAME_REGISTRY_H
#define PIPEFRAME_REGISTRY_H


#include "Logger/Logger.h"

#include <deque>
#include <memory>
#include <set>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Entity.h"
#include "Pool.h"
#include "Signature.h"
#include "EntitySystem.h"

class Registry
{
private:
    int numEntities = 0;

    std::set<Entity> entities;
    std::vector<std::shared_ptr<IPool>> componentPools;
    std::vector<Signature> entityComponentSignatures;
    std::unordered_map<std::type_index, std::shared_ptr<EntitySystem>> systems;
    std::unordered_set<std::type_index> automaticSystemTypes;

    std::set<Entity> entitiesToBeAdded;
    std::set<Entity> entitiesToBeKilled;

    std::unordered_map<std::string, Entity> entityPerTag;
    std::unordered_map<int, std::string> tagPerEntity;

    std::unordered_map<std::string, std::set<Entity>> entitiesPerGroup;
    std::unordered_map<int, std::string> groupPerEntity;

    std::deque<int> freeIds;

public:
    Registry();
    ~Registry();

    std::vector<Entity> GetAllEntities() const;
    void Update();

    Entity CreateEntity();
    void KillEntity(Entity entity);

    void TagEntity(Entity entity, const std::string& tag);
    bool EntityHasTag(Entity entity, const std::string& tag) const;
    std::string GetEntityTag(Entity entity) const;
    Entity GetEntityByTag(const std::string& tag) const;
    void RemoveEntityTag(Entity entity);

    void GroupEntity(Entity entity, const std::string& group);
    bool EntityBelongsToGroup(Entity entity, const std::string& group) const;
    std::string GetEntityGroup(Entity entity) const;
    std::vector<Entity> GetEntitiesByGroup(const std::string& group) const;
    void RemoveEntityGroup(Entity entity);

    template <typename TComponent, typename... TArgs>
    void AddComponent(Entity entity, TArgs&&... args);

    template <typename TComponent>
    void RemoveComponent(Entity entity);

    template <typename TComponent>
    bool HasComponent(Entity entity) const;

    template <typename TComponent>
    TComponent& GetComponent(Entity entity) const;

    template <typename TSystem, typename... TArgs>
    void AddSystem(TArgs&&... args);

    template <typename TSystem, typename... TArgs>
    void AddManualSystem(TArgs&&... args);

    template <typename TSystem>
    void RemoveSystem();

    template <typename TSystem>
    bool HasSystem() const;

    template <typename TSystem>
    TSystem& GetSystem() const;

    void LoadedSystems();
    void StartSystems(EntitySystemContext& context);
    void SubscribeSystems(EntitySystemContext& context);
    void UpdateAutomaticSystems(EntitySystemContext& context);
    void StopSystems(EntitySystemContext& context);
    void UnloadedSystems(EntitySystemContext& context);

    void AddEntityToSystems(Entity entity);
    void RemoveEntityFromSystems(Entity entity);
};

template <typename TSystem, typename... TArgs>
void Registry::AddSystem(TArgs&&... args)
{
    const std::type_index systemType(typeid(TSystem));
    std::shared_ptr<TSystem> newSystem = std::make_shared<TSystem>(std::forward<TArgs>(args)...);
    systems.insert_or_assign(systemType, newSystem);
    automaticSystemTypes.insert(systemType);
}

template <typename TSystem, typename... TArgs>
void Registry::AddManualSystem(TArgs&&... args)
{
    const std::type_index systemType(typeid(TSystem));
    std::shared_ptr<TSystem> newSystem = std::make_shared<TSystem>(std::forward<TArgs>(args)...);
    systems.insert_or_assign(systemType, newSystem);
    automaticSystemTypes.erase(systemType);
}

template <typename TSystem>
void Registry::RemoveSystem()
{
    const std::type_index systemType(typeid(TSystem));
    auto system = systems.find(systemType);
    systems.erase(system);
    automaticSystemTypes.erase(systemType);
}

template <typename TSystem>
bool Registry::HasSystem() const
{
    return systems.find(std::type_index(typeid(TSystem))) != systems.end();
}

template <typename TSystem>
TSystem& Registry::GetSystem() const
{
    auto system = systems.find(std::type_index(typeid(TSystem)));
    return *(std::static_pointer_cast<TSystem>(system->second));
}

template <typename TComponent, typename... TArgs>
void Registry::AddComponent(Entity entity, TArgs&&... args)
{
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    if (componentId >= static_cast<int>(componentPools.size()))
    {
        componentPools.resize(componentId + 1, nullptr);
    }

    if (!componentPools[componentId])
    {
        std::shared_ptr<Pool<TComponent>> newComponentPool(new Pool<TComponent>());
        componentPools[componentId] = newComponentPool;
    }

    std::shared_ptr<Pool<TComponent>> componentPool =
        std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);

    TComponent newComponent(std::forward<TArgs>(args)...);
    componentPool->Set(entityId, newComponent);

    entityComponentSignatures[entityId].set(componentId);
    AddEntityToSystems(entity);

    Logger::Log(
        "Component id = " + std::to_string(componentId) +
        " was added to entity id " + std::to_string(entityId)
    );
}

template <typename TComponent>
void Registry::RemoveComponent(Entity entity)
{
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    std::shared_ptr<Pool<TComponent>> componentPool =
        std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);

    componentPool->Remove(entityId);

    entityComponentSignatures[entityId].set(componentId, false);
    AddEntityToSystems(entity);

    Logger::Log(
        "Component id = " + std::to_string(componentId) +
        " was removed from entity id " + std::to_string(entityId)
    );
}

template <typename TComponent>
bool Registry::HasComponent(Entity entity) const
{
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    return entityComponentSignatures[entityId].test(componentId);
}

template <typename TComponent>
TComponent& Registry::GetComponent(Entity entity) const
{
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    auto componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);
    return componentPool->Get(entityId);
}

#include "Entity.inl"

#endif //PIPEFRAME_REGISTRY_H
