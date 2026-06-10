

#ifndef PIPEFRAME_COLLISIONEVENT_H
#define PIPEFRAME_COLLISIONEVENT_H
#include "ECS/ECS.h"
#include "EventBus/Event.h"

class CollisionEvent : public Event
{
public:
    Entity entityA;
    Entity entityB;

    CollisionEvent(Entity entityA, Entity entityB) : entityA(entityA), entityB(entityB)
    {
    }

    template <typename TComponent>
    bool Has() const
    {
        return entityA.HasComponent<TComponent>() || entityB.HasComponent<TComponent>();
    }

    template <typename TComponent>
    Entity GetEntityWith() const
    {
        if (entityA.HasComponent<TComponent>())
        {
            return entityA;
        }

        if (entityB.HasComponent<TComponent>())
        {
            return entityB;
        }

        return Entity(-1);
    }

    Entity GetOtherEntity(Entity entity) const
    {
        if (entityA == entity)
        {
            return entityB;
        }

        if (entityB == entity)
        {
            return entityA;
        }

        return Entity(-1);
    }

    template <typename TComponentA, typename TComponentB>
    bool Matches() const
    {
        return (
            entityA.HasComponent<TComponentA>() &&
            entityB.HasComponent<TComponentB>()
        ) || (
            entityA.HasComponent<TComponentB>() &&
            entityB.HasComponent<TComponentA>()
        );
    }

    template <typename TComponentA, typename TComponentB>
    bool TryGetPair(Entity& entityWithA, Entity& entityWithB) const
    {
        if (
            entityA.HasComponent<TComponentA>() &&
            entityB.HasComponent<TComponentB>()
        )
        {
            entityWithA = entityA;
            entityWithB = entityB;
            return true;
        }

        if (
            entityA.HasComponent<TComponentB>() &&
            entityB.HasComponent<TComponentA>()
        )
        {
            entityWithA = entityB;
            entityWithB = entityA;
            return true;
        }

        return false;
    }
};

#endif //PIPEFRAME_COLLISIONEVENT_H
