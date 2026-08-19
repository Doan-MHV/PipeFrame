#ifndef PIPEFRAME_SCENE_H
#define PIPEFRAME_SCENE_H

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>

#include "PipeFrame/Render/RenderContext.h"

class Scene {
  public:
    virtual ~Scene() = default;

    virtual void Load() {}

    virtual void Start() {}

    virtual void OnResize(sf::Vector2u newSize, RenderContext &context) {
        (void)newSize;
        (void)context;
    }

    virtual void HandleEvent(const sf::Event &event, RenderContext &context) {
        (void)event;
        (void)context;
    }

    virtual void FixedUpdate(float fixedDeltaTime) { (void)fixedDeltaTime; }

    virtual void Update(float deltaTime) { (void)deltaTime; }

    virtual void Render(RenderContext &context) { (void)context; }

    virtual void Stop() {}

    virtual void Unload() {}
};

#endif
