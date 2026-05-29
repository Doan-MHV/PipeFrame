

#include "Registry.h"

#include <algorithm>

Registry::Registry()
{
    Logger::Log("Registry constructor called");
}

Registry::~Registry()
{
    Logger::Log("Registry destructor called");
}

std::vector<Entity> Registry::GetAllEntities() const
{
    return std::vector<Entity>(entities.begin(), entities.end());
}

Entity Registry::CreateEntity()
{
    int entityId;

    if (freeIds.empty())
    {
        entityId = numEntities++;

        if (entityId >= static_cast<int>(entityComponentSignatures.size()))
        {
            entityComponentSignatures.resize(entityId + 1);
        }
    }
    else
    {
        entityId = freeIds.front();
        freeIds.pop_front();
    }

    Entity entity(entityId);
    entity.registry = this;
    entities.insert(entity);
    entitiesToBeAdded.insert(entity);

    Logger::Log("Entity created with id " + std::to_string(entityId));
    return entity;
}

void Registry::KillEntity(Entity entity)
{
    entitiesToBeKilled.insert(entity);
    Logger::Log("Entity with id " + std::to_string(entity.GetId()) + " was killed");
}

void Registry::AddEntityToSystems(Entity entity)
{
    const auto entityId = entity.GetId();
    const auto& entityComponentSignature = entityComponentSignatures[entityId];

    for (auto& system : systems)
    {
        system.second->RemoveEntityFromSystem(entity);

        const auto& systemComponentSignature = system.second->GetComponentSignature();
        bool isInterested = (entityComponentSignature & systemComponentSignature) == systemComponentSignature;

        if (isInterested)
        {
            system.second->AddEntityToSystem(entity);
        }
    }
}

void Registry::RemoveEntityFromSystems(Entity entity)
{
    for (auto& system : systems)
    {
        system.second->RemoveEntityFromSystem(entity);
    }
}

void Registry::TagEntity(Entity entity, const std::string& tag)
{
    entityPerTag.emplace(tag, entity);
    tagPerEntity.emplace(entity.GetId(), tag);
}

bool Registry::EntityHasTag(Entity entity, const std::string& tag) const
{
    if (tagPerEntity.find(entity.GetId()) == tagPerEntity.end())
    {
        return false;
    }

    return entityPerTag.find(tag)->second == entity;
}

std::string Registry::GetEntityTag(Entity entity) const
{
    const auto taggedEntity = tagPerEntity.find(entity.GetId());

    if (taggedEntity == tagPerEntity.end())
    {
        return "";
    }

    return taggedEntity->second;
}

Entity Registry::GetEntityByTag(const std::string& tag) const
{
    return entityPerTag.at(tag);
}

void Registry::RemoveEntityTag(Entity entity)
{
    auto taggedEntity = tagPerEntity.find(entity.GetId());

    if (taggedEntity != tagPerEntity.end())
    {
        auto tag = taggedEntity->second;
        entityPerTag.erase(tag);
        tagPerEntity.erase(taggedEntity);
    }
}

void Registry::GroupEntity(Entity entity, const std::string& group)
{
    entitiesPerGroup.emplace(group, std::set<Entity>());
    entitiesPerGroup[group].emplace(entity);
    groupPerEntity.emplace(entity.GetId(), group);
}

bool Registry::EntityBelongsToGroup(Entity entity, const std::string& group) const
{
    if (entitiesPerGroup.find(group) == entitiesPerGroup.end())
    {
        return false;
    }

    auto groupEntities = entitiesPerGroup.at(group);
    return groupEntities.contains(entity);
}

std::string Registry::GetEntityGroup(Entity entity) const
{
    const auto groupedEntity = groupPerEntity.find(entity.GetId());

    if (groupedEntity == groupPerEntity.end())
    {
        return "";
    }

    return groupedEntity->second;
}

std::vector<Entity> Registry::GetEntitiesByGroup(const std::string& group) const
{
    auto& setOfEntities = entitiesPerGroup.at(group);
    return std::vector<Entity>(setOfEntities.begin(), setOfEntities.end());
}

void Registry::RemoveEntityGroup(Entity entity)
{
    auto groupedEntity = groupPerEntity.find(entity.GetId());

    if (groupedEntity != groupPerEntity.end())
    {
        auto group = entitiesPerGroup.find(groupedEntity->second);

        if (group != entitiesPerGroup.end())
        {
            auto entityInGroup = group->second.find(entity);

            if (entityInGroup != group->second.end())
            {
                group->second.erase(entityInGroup);
            }
        }

        groupPerEntity.erase(groupedEntity);
    }
}

void Registry::Update()
{
    for (auto entity : entitiesToBeAdded)
    {
        AddEntityToSystems(entity);
    }
    entitiesToBeAdded.clear();

    for (auto entity : entitiesToBeKilled)
    {
        RemoveEntityFromSystems(entity);
        entityComponentSignatures[entity.GetId()].reset();

        for (auto& pool : componentPools)
        {
            if (pool)
            {
                pool->RemoveEntityFromPool(entity.GetId());
            }
        }

        freeIds.push_back(entity.GetId());
        RemoveEntityTag(entity);
        RemoveEntityGroup(entity);
        entities.erase(entity);
    }

    entitiesToBeKilled.clear();
}
