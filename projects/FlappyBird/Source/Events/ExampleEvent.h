#ifndef PIPEFRAME_EXAMPLEEVENT_H
#define PIPEFRAME_EXAMPLEEVENT_H

#include <string>
#include <utility>

#include "ECS/Entity.h"
#include "EventBus/Event.h"

class ExampleEvent : public Event
{
public:
    Entity entity;
    std::string message;

    ExampleEvent(Entity entity, std::string message)
        : entity(entity), message(std::move(message))
    {
    }
};

#endif // PIPEFRAME_EXAMPLEEVENT_H
