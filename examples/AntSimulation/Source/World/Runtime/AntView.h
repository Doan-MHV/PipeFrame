#pragma once
#include <PipeFrame/ECS/ComponentView.h>
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Components/EnergyComponent.h>
#include "Components/AntIdentityComponent.h"
#include "Components/AntPoseComponent.h"
#include "Components/AntEncounterComponent.h"
#include "Components/ForagingComponent.h"
#include "Configuration/AntConfiguration.h"
namespace ant_simulation {
// Non-owning scene view. Copies refer to the same entity; components own all state.
class AntView : public pipeframe::ComponentView<AntIdentityComponent, AntPoseComponent, pipeframe::EnergyComponent, ForagingComponent, AntEncounterComponent, pipeframe::Transform2DComponent, pipeframe::Motion2DComponent> {
  public:
    static constexpr float MarkerTimeoutCoefficient{
        10.0f
    };

    static constexpr float BaseMass{
        0.1f
    };

    static constexpr float FoodMass{
        0.2f
    };

    explicit AntView(pipeframe::SceneObject object) : ComponentView(object) {}
    pipeframe::SceneObject GetObject() const { return object; }
    void Initialize(
        AntId id,
        ColonyId colonyId,
        AntRole role,
        pipeframe::Vector2f position,
        float initialAngle,
        float initialMarkerOffset,
        const AntConfiguration &configuration
    );

    void Update(float deltaTime);

    void UpdateLegs(float deltaTime);

    void SetPosition(pipeframe::Vector2f position);

    void SetDirection(
        pipeframe::Vector2f direction
    );

    void SetAngleInstant(float angle);

    void SetTarget(
        pipeframe::Vector2f target
    );

    void SetTarget(
        pipeframe::Vector2f target,
        float distance
    );

    void SetState(ForagingState state);

    [[nodiscard]]
    bool IsTargetReached() const;

    [[nodiscard]]
    bool IsMarkerReady(
        float markerDistance
    ) const;

    [[nodiscard]]
    MarkerKind GetDropMarkerKind() const;

    [[nodiscard]]
    MarkerKind DropMarker();

    [[nodiscard]]
    MarkerKind GetMarkerFocus() const;

    [[nodiscard]]
    bool IsBlocked(
        const AntConfiguration &configuration
    ) const;

    [[nodiscard]]
    float GetBlockedRatio(
        const AntConfiguration &configuration
    ) const;

    [[nodiscard]]
    float GetMarkerIntensity(
        const AntConfiguration &configuration
    ) const;

    [[nodiscard]]
    float GetEnemyMarkerIntensity(
        const AntConfiguration &configuration
    ) const;

    [[nodiscard]]
    bool IsDead() const;

    void Kill();

    void ConsumeEnergy(float amount);

    void RefillEnergy(
        const AntConfiguration &configuration
    );

    [[nodiscard]]
    bool IsCarryingFood() const;

    [[nodiscard]]
    float GetMass() const;

    void BeginEncounter(AntId opponent);

    void EndEncounter();

    [[nodiscard]]
    bool IsInEncounter() const;

    [[nodiscard]]
    float GetAngle() const;

    [[nodiscard]]
    pipeframe::Vector2f GetDirection() const;

    [[nodiscard]]
    AntId GetId() const;

    [[nodiscard]]
    ColonyId GetColonyId() const;

    [[nodiscard]]
    AntRole GetRole() const;

    [[nodiscard]]
    ForagingState GetState() const;

    [[nodiscard]]
    pipeframe::Vector2f GetPosition() const;

    [[nodiscard]] AntId GetAgentId() const { return ComponentView::GetEntity(); }
    [[nodiscard]] pipeframe::Vector2f GetVelocity() const;
    [[nodiscard]] bool IsActive() const { return !IsDead(); }

    [[nodiscard]]
    pipeframe::Vector2f GetTarget() const;

    [[nodiscard]]
    float GetDistanceToTarget() const;

    [[nodiscard]]
    float GetEnergy() const;

    [[nodiscard]]
    const std::array<AntLegPose, 6> &GetLegs() const;

    [[nodiscard]]
    std::array<AntLegPose, 6> &GetLegs();

    pipeframe::EnergyComponent &GetEnergyComponent() const { return Require<pipeframe::EnergyComponent>(); }
    ForagingComponent &GetForagingComponent() const { return Require<ForagingComponent>(); }
    AntEncounterComponent &GetEncounterComponent() const { return Require<AntEncounterComponent>(); }
    AntIdentityComponent &Identity() const { return Require<AntIdentityComponent>(); }
    AntPoseComponent &Pose() const { return Require<AntPoseComponent>(); }
    pipeframe::Transform2DComponent &Transform() const { return Require<pipeframe::Transform2DComponent>(); }
    pipeframe::Motion2DComponent &Motion() const { return Require<pipeframe::Motion2DComponent>(); }
private:

    void CreateLegs();
    static pipeframe::Vector2f Normalize(pipeframe::Vector2f);
    static float Distance(pipeframe::Vector2f, pipeframe::Vector2f);
};
} // namespace ant_simulation
