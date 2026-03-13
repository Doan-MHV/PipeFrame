#include "ECS.h"
#include "Logger/Logger.h"

int IComponent::nextId = 0;

int Entity::GetId() const
{
    return id;
}

void System::AddEntityToSystem(Entity entity)
{
    entities.push_back(entity);
}

void System::RemoveEntityFromSystem(Entity entity)
{
    std::erase_if(entities,
                  [&entity](const Entity& e) { return e == entity; });
}

const Signature& System::GetComponentSignature() const
{
    return componentSignature;
}

std::vector<Entity> System::GetSystemEntities() const
{
    return entities;
}

Entity Registry::CreateEntity()
{
    int entityId;

    entityId = numEntities++;

    Entity entity(entityId);
    entity.registry = this;
    entitiesToBeAdded.insert(entity);

    if (entityId >= entityComponentSignatures.size())
    {
        entityComponentSignatures.resize(entityId + 1);
    }

    PF_INFO("Entity created with id = " + std::to_string(entityId));

    return entity;
}

void Registry::AddEntityToSystem(Entity entity)
{
    const auto entityId = entity.GetId();

    const auto& entityComponentSignature = entityComponentSignatures[entityId];

    for (auto& system : systems)
    {
        const auto& systemComponentSignature = system.second->GetComponentSignature();

        if (bool isInterested = (entityComponentSignature & systemComponentSignature) == systemComponentSignature)
        {
            system.second->AddEntityToSystem(entity);
        }
    }
}

void Registry::Update()
{
    for (const auto entity : entitiesToBeAdded)
    {
        AddEntityToSystem(entity);
    }
    entitiesToBeAdded.clear();
}
