

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
};

#endif //PIPEFRAME_COLLISIONEVENT_H
