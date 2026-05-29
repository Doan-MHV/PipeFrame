#ifndef PIPEFRAME_ENTITY_INL
#define PIPEFRAME_ENTITY_INL

#include "Entity.h"
#include <utility>

template <typename TComponent, typename... TArgs>
void Entity::AddComponent(TArgs&&... args)
{
    registry->AddComponent<TComponent>(*this, std::forward<TArgs>(args)...);
}

template <typename TComponent>
void Entity::RemoveComponent()
{
    registry->RemoveComponent<TComponent>(*this);
}

template <typename TComponent>
bool Entity::HasComponent() const
{
    return registry->HasComponent<TComponent>(*this);
}

template <typename TComponent>
TComponent& Entity::GetComponent() const
{
    return registry->GetComponent<TComponent>(*this);
}

#endif // PIPEFRAME_ENTITY_INL
