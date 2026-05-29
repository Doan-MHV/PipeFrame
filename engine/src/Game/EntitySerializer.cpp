
#include <nlohmann/json.hpp>
#include "EntitySerializer.h"

#include <fstream>
#include <unordered_set>

#include "Components/AnimationComponent.h"
#include "Components/AttributesComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Components/CameraFollowComponent.h"
#include "Components/EditorEntityComponent.h"
#include "Components/HealthComponent.h"
#include "Components/KeyboardControlledComponent.h"
#include "Components/MovementTypeComponent.h"
#include "Components/PersistentIdComponent.h"
#include "Components/ProjectileEmitterComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SoftCollisionComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "Reflection/EditorMetadata.h"

using json = nlohmann::json;

namespace
{
    bool ShouldSaveEntity(Entity entity)
    {
        if (!entity.HasComponent<EditorEntityComponent>())
        {
            return false;
        }

        if (!entity.HasComponent<PersistentIdComponent>())
        {
            return false;
        }

        if (entity.BelongsToGroup("projectiles"))
        {
            return false;
        }

        if (entity.BelongsToGroup("tiles"))
        {
            return false;
        }

        return true;
    }
}

nlohmann::json EntitySerializer::SerializeEntities(const std::unique_ptr<Registry>& registry)
{
    return SerializeEntities(registry, nullptr);
}

nlohmann::json EntitySerializer::SerializeEntities(
    const std::unique_ptr<Registry>& registry,
    const ComponentRegistry* componentRegistry
)
{
    json root;
    root["entities"] = json::array();

    if (!registry)
    {
        return root;
    }

    std::unordered_set<std::string> seenPersistentIds;

    for (auto entity : registry->GetAllEntities())
    {
        if (!ShouldSaveEntity(entity))
        {
            continue;
        }

        const std::string persistentId = entity.GetComponent<PersistentIdComponent>().value;

        if (persistentId.empty())
        {
            Logger::Err("Cannot serialize entity with empty persistent id. Runtime ID: " +
                std::to_string(entity.GetId()));
            continue;
        }

        if (seenPersistentIds.contains(persistentId))
        {
            Logger::Err("Duplicate persistent id detected while serializing entities: " + persistentId);
            continue;
        }

        seenPersistentIds.insert(persistentId);
        root["entities"].push_back(SerializeEntity(entity, componentRegistry));
    }

    return root;
}

nlohmann::json EntitySerializer::SerializeEntity(Entity entity)
{
    return SerializeEntity(entity, nullptr);
}

nlohmann::json EntitySerializer::SerializeEntity(
    Entity entity,
    const ComponentRegistry* componentRegistry
)
{
    json entityJson;
    json componentsJson;

    if (entity.HasComponent<PersistentIdComponent>())
    {
        entityJson["id"] = entity.GetComponent<PersistentIdComponent>().value;
    }
    else
    {
        entityJson["id"] = "entity_" + std::to_string(entity.GetId());
    }

    const std::string tag = entity.registry->GetEntityTag(entity);
    if (!tag.empty())
    {
        entityJson["tag"] = tag;
    }

    const std::string group = entity.registry->GetEntityGroup(entity);
    if (!group.empty())
    {
        entityJson["group"] = group;
    }

    if (componentRegistry)
    {
        entityJson["components"] = SerializeRegisteredComponents(entity, *componentRegistry);
        return entityJson;
    }

    if (entity.HasComponent<TransformComponent>())
    {
        const auto& transform = entity.GetComponent<TransformComponent>();

        componentsJson["transform"] = {
            {
                "position",
                {
                    {"x", transform.position.x},
                    {"y", transform.position.y}
                }
            },
            {
                "scale",
                {
                    {"x", transform.scale.x},
                    {"y", transform.scale.y}
                }
            },
            {"rotation", transform.rotation}
        };
    }

    if (entity.HasComponent<RigidBodyComponent>())
    {
        const auto& rigidbody = entity.GetComponent<RigidBodyComponent>();

        componentsJson["rigidbody"] = {
            {
                "velocity",
                {
                    {"x", rigidbody.velocity.x},
                    {"y", rigidbody.velocity.y}
                }
            }
        };
    }

    if (entity.HasComponent<SpriteComponent>())
    {
        const auto& sprite = entity.GetComponent<SpriteComponent>();

        componentsJson["sprite"] = {
            {"texture_asset_id", sprite.assetId},
            {"width", sprite.width},
            {"height", sprite.height},
            {"z_index", sprite.zIndex},
            {"fixed", sprite.isFixed},
            {"src_rect_x", sprite.srcRect.x},
            {"src_rect_y", sprite.srcRect.y},
            {"src_rect_w", sprite.srcRect.w},
            {"src_rect_h", sprite.srcRect.h}
        };
    }

    if (entity.HasComponent<MovementTypeComponent>())
    {
        const auto& movement = entity.GetComponent<MovementTypeComponent>();

        componentsJson["movement"] = {
            {"type", MovementTypeToString(movement.type)}
        };
    }

    if (entity.HasComponent<HealthComponent>())
    {
        const auto& health = entity.GetComponent<HealthComponent>();

        componentsJson["health"] = {
            {"health_percentage", health.healthPercentage}
        };
    }

    if (entity.HasComponent<AttributesComponent>())
    {
        const auto& attributes = entity.GetComponent<AttributesComponent>();
        componentsJson["attributes"] = attributes.values;
    }

    if (entity.HasComponent<BoxColliderComponent>())
    {
        const auto& collider = entity.GetComponent<BoxColliderComponent>();

        componentsJson["boxcollider"] = {
            {"width", collider.width},
            {"height", collider.height},
            {
                "offset",
                {
                    {"x", collider.offset.x},
                    {"y", collider.offset.y}
                }
            }
        };
    }

    if (entity.HasComponent<SoftCollisionComponent>())
    {
        const auto& softCollision = entity.GetComponent<SoftCollisionComponent>();

        componentsJson["soft_collision"] = {
            {"radius", softCollision.radius},
            {"push_strength", softCollision.pushStrength},
            {"immovable", softCollision.immovable}
        };
    }

    if (entity.HasComponent<ProjectileEmitterComponent>())
    {
        const auto& projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();

        componentsJson["projectile_emitter"] = {
            {
                "projectile_velocity",
                {
                    {"x", projectileEmitter.projectileVelocity.x},
                    {"y", projectileEmitter.projectileVelocity.y}
                }
            },
            {"repeat_frequency", projectileEmitter.repeatFrequency / 1000},
            {"projectile_duration", projectileEmitter.projectileDuration / 1000},
            {"hit_percentage_damage", projectileEmitter.hitPercentDamage},
            {"friendly", projectileEmitter.isFriendly}
        };
    }

    if (entity.HasComponent<AnimationComponent>())
    {
        const auto& animation = entity.GetComponent<AnimationComponent>();

        componentsJson["animation"] = {
            {"num_frames", animation.numFrames},
            {"speed_rate", animation.frameSpeedRate}
        };
    }

    if (entity.HasComponent<CameraFollowComponent>())
    {
        componentsJson["camera_follow"] = json::object();
    }

    if (entity.HasComponent<KeyboardControlledComponent>())
    {
        const auto& keyboard = entity.GetComponent<KeyboardControlledComponent>();

        componentsJson["keyboard_controller"] = {
            {
                "up_velocity",
                {
                    {"x", keyboard.upVelocity.x},
                    {"y", keyboard.upVelocity.y}
                }
            },
            {
                "right_velocity",
                {
                    {"x", keyboard.rightVelocity.x},
                    {"y", keyboard.rightVelocity.y}
                }
            },
            {
                "down_velocity",
                {
                    {"x", keyboard.downVelocity.x},
                    {"y", keyboard.downVelocity.y}
                }
            },
            {
                "left_velocity",
                {
                    {"x", keyboard.leftVelocity.x},
                    {"y", keyboard.leftVelocity.y}
                }
            }
        };
    }

    entityJson["components"] = componentsJson;
    return entityJson;
}

bool EntitySerializer::SaveEntities(const std::unique_ptr<Registry>& registry, const std::string& filePath)
{
    return SaveEntities(registry, filePath, nullptr);
}

bool EntitySerializer::SaveEntities(
    const std::unique_ptr<Registry>& registry,
    const std::string& filePath,
    const ComponentRegistry* componentRegistry
)
{
    json root = SerializeEntities(registry, componentRegistry);

    std::ofstream output(filePath);
    if (!output.is_open())
    {
        return false;
    }

    output << root.dump(4);
    return true;
}
