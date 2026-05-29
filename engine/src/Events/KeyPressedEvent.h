

#ifndef PIPEFRAME_KEYPRESSEDEVENT_H
#define PIPEFRAME_KEYPRESSEDEVENT_H
#include <SDL3/SDL_keycode.h>

#include "EventBus/Event.h"

class KeyPressedEvent : public Event
{
public:
    SDL_Keycode symbol;

    KeyPressedEvent(SDL_Keycode symbol) : symbol(symbol)
    {
    }
};

#endif //PIPEFRAME_KEYPRESSEDEVENT_H
