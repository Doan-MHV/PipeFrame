

#ifndef PIPEFRAME_KEYBOARDCONTROLSYSTEM_H
#define PIPEFRAME_KEYBOARDCONTROLSYSTEM_H

#include <SDL3/SDL.h>

#include "Components/KeyboardControlledComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/RigidBodyComponent.h"
#include "ECS/ECS.h"
#include "EventBus/EventBus.h"

class KeyboardControlSystem : public EntitySystem
{
public:
    KeyboardControlSystem()
    {
        RequireComponent<KeyboardControlledComponent>();
        RequireComponent<SpriteComponent>();
        RequireComponent<RigidBodyComponent>();
    }

    void SubscribeToEvents(std::unique_ptr<EventBus>&)
    {
    }

    void Update()
    {
        const bool* keys = SDL_GetKeyboardState(nullptr);

        for (auto entity : GetSystemEntities())
        {
            const auto keyboardControl = entity.GetComponent<KeyboardControlledComponent>();
            auto& sprite = entity.GetComponent<SpriteComponent>();
            auto& rigidBody = entity.GetComponent<RigidBodyComponent>();

            rigidBody.velocity = glm::vec2(0.0f);

            if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])
            {
                rigidBody.velocity = keyboardControl.upVelocity;
                sprite.srcRect.y = sprite.height * 0;
            }
            else if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
            {
                rigidBody.velocity = keyboardControl.rightVelocity;
                sprite.srcRect.y = sprite.height * 1;
            }
            else if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
            {
                rigidBody.velocity = keyboardControl.downVelocity;
                sprite.srcRect.y = sprite.height * 2;
            }
            else if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])
            {
                rigidBody.velocity = keyboardControl.leftVelocity;
                sprite.srcRect.y = sprite.height * 3;
            }
        }
    }
};

#endif //PIPEFRAME_KEYBOARDCONTROLSYSTEM_H
