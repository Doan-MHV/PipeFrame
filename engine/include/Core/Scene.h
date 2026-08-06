#ifndef PIPEFRAME_SCENE_H
#define PIPEFRAME_SCENE_H

#include <SFML/Window/Event.hpp>

#include "Render/RenderContext.h"

class Scene
{
public:
    virtual ~Scene() = default;

    virtual void Load()
    {
    }

    virtual void Start()
    {
    }

    virtual void HandleEvent(const sf::Event& event)
    {
        (void)event;
    }

    virtual void Update(float deltaTime)
    {
        (void)deltaTime;
    }

    virtual void Render(RenderContext& context)
    {
        (void)context;
    }

    virtual void Stop()
    {
    }

    virtual void Unload()
    {
    }
};

#endif
