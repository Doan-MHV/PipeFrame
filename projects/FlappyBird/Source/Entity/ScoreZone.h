#ifndef FLAPPY_BIRD_SCORE_ZONE_H
#define FLAPPY_BIRD_SCORE_ZONE_H

#include <glm/glm.hpp>

#include "Components/BoxColliderComponent.h"
#include "Components/EditorEntityComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/ScoreZoneComponent.h"
#include "Components/TransformComponent.h"
#include "Components/TriggerComponent.h"
#include "ECS/ECS.h"
#include "Project/EntityIdGenerator.h"

struct ScoreZone
{
    static Entity Create(Registry& registry, glm::vec2 position)
    {
        Entity entity = registry.CreateEntity();

        entity.AddComponent<EditorEntityComponent>();
        entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, "ScoreZone"));
        entity.AddComponent<TransformComponent>(position);
        entity.AddComponent<BoxColliderComponent>(
            48,
            160,
            glm::vec2(0.0f),
            false,
            true
        );
        entity.AddComponent<TriggerComponent>("score", true);
        entity.AddComponent<ScoreZoneComponent>();

        entity.Group("score_zones");

        return entity;
    }
};

#endif // FLAPPY_BIRD_SCORE_ZONE_H
