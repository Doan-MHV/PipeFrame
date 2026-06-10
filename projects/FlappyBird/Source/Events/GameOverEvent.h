#ifndef GameOverEvent_H
#define GameOverEvent_H

#include "ECS/Entity.h"
#include "EventBus/Event.h"

struct GameOverEvent : public Event
{
    Entity entity{-1};
    std::string reason;

    GameOverEvent() = default;

    explicit GameOverEvent(Entity entity, std::string reason) : entity{entity}, reason{std::move(reason)}
    {
    }
};

#endif // GameOverEvent_H
