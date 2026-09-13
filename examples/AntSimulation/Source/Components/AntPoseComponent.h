#pragma once
#include <PipeFrame/Project/ComponentSchema.h>

#include <PipeFrame/Simulation/DirectionTracker.h>
#include <array>
namespace ant_simulation {

class AntLegPose {
public:
    static constexpr float InterpolationSpeed{
        15.0f
    };

    void Initialize(
        pipeframe::Vector2f relativeStart,
        pipeframe::Vector2f relativeEnd,
        pipeframe::Vector2f initialWorldPosition
    );

    void Advance(float deltaTime);

    void UpdateReference(
        pipeframe::Vector2f antPosition,
        float antAngle
    );

    [[nodiscard]]
    bool IsDone() const;

    [[nodiscard]]
    pipeframe::Vector2f GetRelativeStart() const;

    [[nodiscard]]
    pipeframe::Vector2f GetRelativeEnd() const;

    [[nodiscard]]
    pipeframe::Vector2f GetWorldStart(
        pipeframe::Vector2f antPosition,
        float antAngle
    ) const;

    [[nodiscard]]
    pipeframe::Vector2f GetCurrentWorldEnd() const;

    [[nodiscard]]
    pipeframe::Vector2f GetTargetWorldEnd() const;

    [[nodiscard]]
    float GetReferenceDistance() const;

private:
    static pipeframe::Vector2f TransformPoint(
        pipeframe::Vector2f point,
        pipeframe::Vector2f position,
        float angle
    );

    static float Distance(
        pipeframe::Vector2f first,
        pipeframe::Vector2f second
    );

    void SetTargetWorldEnd(
        pipeframe::Vector2f target
    );

    pipeframe::Vector2f relativeStart{
        0.0f,
        0.0f,
    };

    pipeframe::Vector2f relativeEnd{
        0.0f,
        0.0f,
    };

    pipeframe::Vector2f interpolationStart{
        0.0f,
        0.0f,
    };

    pipeframe::Vector2f currentWorldEnd{
        0.0f,
        0.0f,
    };

    pipeframe::Vector2f targetWorldEnd{
        0.0f,
        0.0f,
    };

    float interpolationProgress{1.0f};
    float referenceDistance{0.0f};
};


struct AntPoseComponent {
    // Inspector exposure is declared here; unlisted members stay runtime-only.
    static auto Schema() {
        using namespace pipeframe;
        using K=PropertyKind;
        return ComponentSchema<AntPoseComponent>("ant.pose","Pose").Required()
        .ReadOnly({.key="heading", .displayName="Heading", .kind=K::Number, .defaultValue=0.0, .unit="radians"},
            [](const auto &c)->PropertyValue { return double(c.direction.GetAngle()); });
    }

    pipeframe::DirectionTracker direction, headDirection, tailDirection;
    std::array<AntLegPose, 6> legs;
};
}
