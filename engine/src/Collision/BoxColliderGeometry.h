#ifndef PIPEFRAME_BOXCOLLIDERGEOMETRY_H
#define PIPEFRAME_BOXCOLLIDERGEOMETRY_H

#include <algorithm>
#include <array>
#include <cmath>

#include <SDL3/SDL_rect.h>
#include <glm/glm.hpp>

#include "Components/BoxColliderComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/Entity.h"

struct BoxColliderGeometry
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
    float rotationDegrees = 0.0f;
    bool rotated = false;
};

inline glm::vec2 GetBoxColliderWorldSize(Entity entity, const TransformComponent& transform)
{
    const auto& collider = entity.GetComponent<BoxColliderComponent>();

    float width = static_cast<float>(collider.width);
    float height = static_cast<float>(collider.height);

    if (collider.matchSpriteSize && entity.HasComponent<SpriteComponent>())
    {
        const auto& sprite = entity.GetComponent<SpriteComponent>();
        width = static_cast<float>(sprite.width);
        height = static_cast<float>(sprite.height);
    }

    return {
        std::max(1.0f, width * transform.scale.x),
        std::max(1.0f, height * transform.scale.y)
    };
}

inline glm::vec2 GetBoxColliderWorldSize(Entity entity)
{
    return GetBoxColliderWorldSize(entity, entity.GetComponent<TransformComponent>());
}

inline BoxColliderGeometry GetBoxColliderGeometry(Entity entity, const TransformComponent& transform)
{
    const auto& collider = entity.GetComponent<BoxColliderComponent>();
    const glm::vec2 size = GetBoxColliderWorldSize(entity, transform);

    BoxColliderGeometry geometry;
    geometry.x = transform.position.x + collider.offset.x;
    geometry.y = transform.position.y + collider.offset.y;
    geometry.width = size.x;
    geometry.height = size.y;
    geometry.rotationDegrees = static_cast<float>(transform.rotation);
    geometry.rotated = collider.rotateWithTransform && std::abs(geometry.rotationDegrees) > 0.001f;

    if (geometry.width < 0.0f)
    {
        geometry.x += geometry.width;
        geometry.width = -geometry.width;
    }

    if (geometry.height < 0.0f)
    {
        geometry.y += geometry.height;
        geometry.height = -geometry.height;
    }

    return geometry;
}

inline BoxColliderGeometry GetBoxColliderGeometry(Entity entity)
{
    return GetBoxColliderGeometry(entity, entity.GetComponent<TransformComponent>());
}

inline std::array<glm::vec2, 4> GetBoxColliderCorners(const BoxColliderGeometry& geometry)
{
    std::array<glm::vec2, 4> corners = {
        glm::vec2(geometry.x, geometry.y),
        glm::vec2(geometry.x + geometry.width, geometry.y),
        glm::vec2(geometry.x + geometry.width, geometry.y + geometry.height),
        glm::vec2(geometry.x, geometry.y + geometry.height)
    };

    if (!geometry.rotated)
    {
        return corners;
    }

    constexpr float degreesToRadians = 3.14159265358979323846f / 180.0f;
    const float radians = geometry.rotationDegrees * degreesToRadians;
    const float cosAngle = std::cos(radians);
    const float sinAngle = std::sin(radians);
    const glm::vec2 center(
        geometry.x + geometry.width * 0.5f,
        geometry.y + geometry.height * 0.5f
    );

    for (auto& corner : corners)
    {
        const glm::vec2 local = corner - center;
        corner = {
            center.x + local.x * cosAngle - local.y * sinAngle,
            center.y + local.x * sinAngle + local.y * cosAngle
        };
    }

    return corners;
}

inline SDL_FRect GetBoxColliderAABB(const BoxColliderGeometry& geometry)
{
    const auto corners = GetBoxColliderCorners(geometry);

    float minX = corners[0].x;
    float minY = corners[0].y;
    float maxX = corners[0].x;
    float maxY = corners[0].y;

    for (const auto& corner : corners)
    {
        minX = std::min(minX, corner.x);
        minY = std::min(minY, corner.y);
        maxX = std::max(maxX, corner.x);
        maxY = std::max(maxY, corner.y);
    }

    return {
        minX,
        minY,
        std::max(1.0f, maxX - minX),
        std::max(1.0f, maxY - minY)
    };
}

#endif // PIPEFRAME_BOXCOLLIDERGEOMETRY_H
