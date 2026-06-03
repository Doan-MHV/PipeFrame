#ifndef Bird_H
#define Bird_H

#include <glm/glm.hpp>

#include "Components/BirdComponent.h"
#include "Components/TransformComponent.h"
#include "Components/EditorEntityComponent.h"
#include "Components/MovementComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SpriteComponent.h"
#include "ECS/ECS.h"
#include "Project/EntityIdGenerator.h"

struct Bird
{
    static Entity Create(Registry& registry, glm::vec2 position)
    {
        Entity entity = registry.CreateEntity();
        entity.AddComponent<EditorEntityComponent>();
        entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, "Bird"));
        entity.AddComponent<TransformComponent>(position, glm::vec2(1.0f, 1.0f), 0.0);
        entity.AddComponent<RigidBodyComponent>();
        entity.AddComponent<MovementComponent>();
        entity.AddComponent<BirdComponent>();
        entity.AddComponent<SpriteComponent>("flappy-bird-texture", 48, 48, 10, false, 0, 0, 32, 32);

        entity.Group("player");

        return entity;
    }
};

#endif // Bird_H
