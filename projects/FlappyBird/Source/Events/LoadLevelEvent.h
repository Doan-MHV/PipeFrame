#ifndef LoadLevelEvent_H
#define LoadLevelEvent_H

#include <string>
#include <utility>

#include "EventBus/Event.h"

struct LoadLevelEvent : public Event
{
    std::string targetLevel;

    LoadLevelEvent() = default;

    explicit LoadLevelEvent(std::string targetLevel)
        : targetLevel(std::move(targetLevel))
    {
    }
};

#endif // LoadLevelEvent_H
