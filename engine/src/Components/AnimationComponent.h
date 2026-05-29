

#ifndef PIPEFRAME_ANIMATIONCOMPONENT_H
#define PIPEFRAME_ANIMATIONCOMPONENT_H

#include <SDL3/SDL.h>

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine)
struct AnimationComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 1, 512, 1)
    int numFrames;

    PF_PROPERTY(PF::Hidden, PF::RuntimeOnly, 0, 512, 1)
    int currentFrame;

    PF_PROPERTY(PF::Edit, PF::Save, 1, 1000, 1, PF::JsonName("speed_rate"), PF::DisplayName("Frame Speed"))
    int frameSpeedRate;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool isLoop;

    PF_PROPERTY(PF::Hidden, PF::RuntimeOnly, 0, 0, 1)
    int startTime;

    AnimationComponent(int numFrames = 1, int frameSpeedRate = 1, bool isLoop = true)
    {
        this->numFrames = numFrames;
        this->currentFrame = 1;
        this->frameSpeedRate = frameSpeedRate;
        this->isLoop = isLoop;
        this->startTime = SDL_GetTicks();
    }
};

#endif //PIPEFRAME_ANIMATIONCOMPONENT_H
