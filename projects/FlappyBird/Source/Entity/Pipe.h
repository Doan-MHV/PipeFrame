#ifndef Pipe_H
#define Pipe_H

#include <glm/glm.hpp>

#include "Components/BoxColliderComponent.h"
#include "Components/HazardComponent.h"
#include "Components/TransformComponent.h"
#include "Components/EditorEntityComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/PipeComponent.h"
#include "ECS/ECS.h"
#include "Project/EntityIdGenerator.h"

struct Pipe
{
    static Entity Create(Registry& registry, glm::vec2 position)
    {
        constexpr int pipeWidth = 64;
        constexpr int pipeHeight = 256;

        Entity entity = registry.CreateEntity();

        entity.AddComponent<EditorEntityComponent>();
        entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, "Pipe"));
        entity.AddComponent<TransformComponent>(position);
        entity.AddComponent<BoxColliderComponent>(
            pipeWidth,
            pipeHeight,
            glm::vec2(0.0f),
            true,
            true
        );
        entity.AddComponent<HazardComponent>();
        entity.AddComponent<PipeComponent>();
        entity.AddComponent<SpriteComponent>("flappy-pipe-texture",
                                             pipeWidth,
                                             pipeHeight,
                                             5,
                                             false,
                                             0,
                                             0,
                                             pipeWidth,
                                             pipeHeight);
        entity.Group("pipe");
        return entity;
    }
};

#endif // Pipe_H
