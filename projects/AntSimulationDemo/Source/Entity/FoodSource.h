#ifndef PIPEFRAME_FOOD_SOURCE_H
#define PIPEFRAME_FOOD_SOURCE_H

#include <glm/glm.hpp>

#include "Components/AttributesComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Components/EditorEntityComponent.h"
#include "Components/FoodSourceComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Project/EntityIdGenerator.h"

inline Entity CreateFoodSourceEntity(Registry& registry, glm::vec2 position)
{
    Entity entity = registry.CreateEntity();
    entity.Group("food");
    entity.AddComponent<EditorEntityComponent>();
    entity.AddComponent<PersistentIdComponent>(BuildUniqueEntityId(registry, "food"));
    entity.AddComponent<TransformComponent>(position);
    entity.AddComponent<SpriteComponent>(
        "marker-texture",
        24,
        24,
        1,
        false,
        0.0f,
        0.0f,
        100.0f,
        100.0f
    );
    entity.AddComponent<BoxColliderComponent>(24, 24);
    entity.AddComponent<AttributesComponent>();
    entity.GetComponent<AttributesComponent>().values["food_amount"] = 25;
    entity.AddComponent<FoodSourceComponent>();
    return entity;
}

#endif // PIPEFRAME_FOOD_SOURCE_H
