#include "EntitySystem.h"

#include <algorithm>

void EntitySystem::AddEntityToSystem(Entity entity) { entities.push_back(entity); }

void EntitySystem::RemoveEntityFromSystem(Entity entity) {
    entities.erase(
        std::remove_if(entities.begin(), entities.end(), [&entity](Entity other) { return entity == other; }),
        entities.end());
}

bool EntitySystem::HasEntity(Entity entity) const {
    return std::find(entities.begin(), entities.end(), entity) != entities.end();
}

const std::vector<Entity> &EntitySystem::GetSystemEntities() const { return entities; }

const Signature &EntitySystem::GetComponentSignature() const { return componentSignature; }
