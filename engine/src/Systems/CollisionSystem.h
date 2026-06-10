

#ifndef PIPEFRAME_COLLISIONSYSTEM_H
#define PIPEFRAME_COLLISIONSYSTEM_H
#include <array>
#include <limits>

#include <glm/geometric.hpp>

#include "Collision/BoxColliderGeometry.h"
#include "Components/BoxColliderComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "EventBus/EventBus.h"
#include "Events/CollisionEvent.h"

class CollisionSystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<TransformComponent>();
        RequireComponent<BoxColliderComponent>();
    }

    void Update(EntitySystemContext& context) override
    {
        auto entities = GetSystemEntities();

        for (auto i = entities.begin(); i != entities.end(); ++i)
        {
            Entity entityA = *i;
            const BoxColliderGeometry aGeometry = GetBoxColliderGeometry(entityA);

            for (auto j = std::next(i); j != entities.end(); ++j)
            {
                Entity entityB = *j;
                const BoxColliderGeometry bGeometry = GetBoxColliderGeometry(entityB);

                if (CheckBoxCollision(aGeometry, bGeometry))
                {
                    Emit<CollisionEvent>(context, entityA, entityB);
                }
            }
        }
    }

    static bool CheckAABBCollision(double aX, double aY, double aW, double aH, double bX, double bY, double bW,
                                   double bH)
    {
        return aX < bX + bW &&
            aX + aW > bX &&
            aY < bY + bH &&
            aY + aH > bY;
    }

    static bool CheckBoxCollision(const BoxColliderGeometry& a, const BoxColliderGeometry& b)
    {
        if (!a.rotated && !b.rotated)
        {
            return CheckAABBCollision(
                a.x,
                a.y,
                a.width,
                a.height,
                b.x,
                b.y,
                b.width,
                b.height
            );
        }

        return CheckOBBCollision(a, b);
    }

private:
    static bool CheckOBBCollision(const BoxColliderGeometry& a, const BoxColliderGeometry& b)
    {
        const auto aCorners = GetBoxColliderCorners(a);
        const auto bCorners = GetBoxColliderCorners(b);

        std::array<glm::vec2, 4> axes = {
            BuildAxis(aCorners[0], aCorners[1]),
            BuildAxis(aCorners[1], aCorners[2]),
            BuildAxis(bCorners[0], bCorners[1]),
            BuildAxis(bCorners[1], bCorners[2])
        };

        for (const auto& axis : axes)
        {
            if (!ProjectionOverlaps(aCorners, bCorners, axis))
            {
                return false;
            }
        }

        return true;
    }

    static glm::vec2 BuildAxis(const glm::vec2& from, const glm::vec2& to)
    {
        const glm::vec2 edge = to - from;
        glm::vec2 axis(-edge.y, edge.x);

        const float length = glm::length(axis);
        if (length <= 0.001f)
        {
            return {1.0f, 0.0f};
        }

        return axis / length;
    }

    static void ProjectCorners(
        const std::array<glm::vec2, 4>& corners,
        const glm::vec2& axis,
        float& min,
        float& max
    )
    {
        min = std::numeric_limits<float>::max();
        max = std::numeric_limits<float>::lowest();

        for (const auto& corner : corners)
        {
            const float projection = glm::dot(corner, axis);
            min = std::min(min, projection);
            max = std::max(max, projection);
        }
    }

    static bool ProjectionOverlaps(
        const std::array<glm::vec2, 4>& aCorners,
        const std::array<glm::vec2, 4>& bCorners,
        const glm::vec2& axis
    )
    {
        float aMin = 0.0f;
        float aMax = 0.0f;
        float bMin = 0.0f;
        float bMax = 0.0f;

        ProjectCorners(aCorners, axis, aMin, aMax);
        ProjectCorners(bCorners, axis, bMin, bMax);

        return aMax >= bMin && bMax >= aMin;
    }
};

#endif //PIPEFRAME_COLLISIONSYSTEM_H
