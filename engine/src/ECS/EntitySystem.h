

#ifndef PIPEFRAME_ENTITYSYSTEM_H
#define PIPEFRAME_ENTITYSYSTEM_H


#include <vector>

#include "Component.h"
#include "Entity.h"
#include "Signature.h"

class EntitySystem
{
private:
    Signature componentSignature;
    std::vector<Entity> entities;

public:
    EntitySystem() = default;
    ~EntitySystem() = default;

    void AddEntityToSystem(Entity entity);
    void RemoveEntityFromSystem(Entity entity);
    std::vector<Entity> GetSystemEntities() const;
    const Signature& GetComponentSignature() const;

    template <typename TComponent>
    void RequireComponent();
};

template <typename TComponent>
void EntitySystem::RequireComponent()
{
    const auto componentId = Component<TComponent>::GetId();
    componentSignature.set(componentId);
}


#endif //PIPEFRAME_ENTITYSYSTEM_H
