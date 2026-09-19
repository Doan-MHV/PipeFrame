#include "World/Runtime/AntView.h"
#include "World/Runtime/Systems/AntPoseAlgorithms.h"

#include <algorithm>
#include <cmath>

namespace ant_simulation {

void AntView::Initialize(const AntId newId, const ColonyId newColonyId, const AntRole newRole,
                         const pipeframe::Vector2f initialPosition, const float initialAngle,
                         const float initialMarkerOffset, const AntConfiguration &configuration) {
    auto &foraging = GetForagingComponent();
    (void)newId;
    Identity().colonyId = newColonyId;
    Identity().role = newRole;
    foraging.state = ForagingState::ToFood;

    Identity().color = configuration.defaultAntColor;

    foraging.walkTime = 0.0f;
    GetEnergyComponent().Refill(configuration.antMaxEnergy);

    Transform().position = initialPosition;
    foraging.target = initialPosition;
    foraging.distanceToTarget = 0.0f;

    foraging.timeSinceLastMarker = 0.0f;
    foraging.blocked = false;

    Motion().speed = 0.0f;
    Motion().travelDistance = 0.0f;
    foraging.collectedFood = 0;

    GetEncounterComponent().enemyTimer = -1.0f;
    GetEncounterComponent().opponentId.reset();

    Identity().physicsObjectId = 0;

    Pose().direction.SetSpeed(4.0f);
    Pose().headDirection.SetSpeed(5.0f);
    Pose().tailDirection.SetSpeed(3.0f);

    SetAngleInstant(initialAngle);
    CreateLegs();

    const float safeMarkerOffset = std::max(0.0f, initialMarkerOffset);

    foraging.lastMarkerPosition = Transform().position - Pose().direction.GetTarget() * safeMarkerOffset;
}

void AntView::Update(const float deltaTime) {
    auto &foraging = GetForagingComponent();
    if (deltaTime <= 0.0f) {
        return;
    }

    AdvanceAntPose(Pose(), Transform(), deltaTime);

    foraging.walkTime += deltaTime;
    foraging.timeSinceLastMarker += deltaTime;

    auto &encounter = GetEncounterComponent();
    if (encounter.enemyTimer >= 0.0f) {
        encounter.enemyTimer += deltaTime;
    }
}

void AntView::UpdateLegs(const float deltaTime) { StepAntLegs(Pose(), Transform(), deltaTime); }

void AntView::SetPosition(const pipeframe::Vector2f newPosition) { Transform().position = newPosition; }

void AntView::SetDirection(const pipeframe::Vector2f newDirection) {
    const pipeframe::Vector2f normalized = Normalize(newDirection);

    Pose().direction.SetTarget(normalized);
    Pose().headDirection.SetTarget(normalized);
    Pose().tailDirection.SetTarget(normalized);
}

void AntView::SetAngleInstant(const float angle) {
    Transform().rotation = angle;
    Pose().direction.SetAngleInstant(angle);
    Pose().headDirection.SetAngleInstant(angle);
    Pose().tailDirection.SetAngleInstant(angle);
}

void AntView::SetTarget(const pipeframe::Vector2f newTarget) {
    SetTarget(newTarget, Distance(Transform().position, newTarget));
}

void AntView::SetTarget(const pipeframe::Vector2f newTarget, const float distance) {
    auto &foraging = GetForagingComponent();
    foraging.target = newTarget;

    foraging.distanceToTarget = std::max(0.0f, distance);

    SetDirection(foraging.target - Transform().position);
}

void AntView::SetState(const ForagingState newState) {
    auto &foraging = GetForagingComponent();
    foraging.state = newState;
}

bool AntView::IsTargetReached() const {
    auto &foraging = GetForagingComponent();
    return foraging.distanceToTarget <= 0.0f;
}

bool AntView::IsMarkerReady(const float markerDistance) const {
    auto &foraging = GetForagingComponent();
    const pipeframe::Vector2f difference = Transform().position - foraging.lastMarkerPosition;

    return difference.x * difference.x + difference.y * difference.y >= markerDistance * markerDistance;
}

MarkerKind AntView::GetDropMarkerKind() const {
    auto &foraging = GetForagingComponent();
    switch (foraging.state) {
    case ForagingState::ToFood:
        return MarkerKind::ToHome;

    case ForagingState::ToHomeWithFood:
        return MarkerKind::ToFood;

    case ForagingState::ToHomeNoFood:
        return MarkerKind::None;
    }

    return MarkerKind::None;
}

MarkerKind AntView::DropMarker() {
    auto &foraging = GetForagingComponent();
    foraging.lastMarkerPosition = Transform().position;
    foraging.timeSinceLastMarker = 0.0f;

    return GetDropMarkerKind();
}

MarkerKind AntView::GetMarkerFocus() const {
    auto &foraging = GetForagingComponent();
    if (Identity().role == AntRole::Soldier) {
        return MarkerKind::ToEnemy;
    }

    switch (foraging.state) {
    case ForagingState::ToHomeWithFood:
    case ForagingState::ToHomeNoFood:
        return MarkerKind::ToHome;

    case ForagingState::ToFood:
        return MarkerKind::ToFood;
    }

    return MarkerKind::ToFood;
}

bool AntView::IsBlocked(const AntConfiguration &configuration) const {
    auto &foraging = GetForagingComponent();
    return foraging.timeSinceLastMarker > MarkerTimeoutCoefficient * configuration.GetAntMarkerInterval();
}

float AntView::GetBlockedRatio(const AntConfiguration &configuration) const {
    auto &foraging = GetForagingComponent();
    const float markerInterval = configuration.GetAntMarkerInterval();

    const float denominator = (MarkerTimeoutCoefficient - 1.0f) * markerInterval;

    if (denominator <= 0.0f) {
        return 0.0f;
    }

    return std::clamp((foraging.timeSinceLastMarker - markerInterval) / denominator, 0.0f, 1.0f);
}

float AntView::GetMarkerIntensity(const AntConfiguration &configuration) const {
    auto &foraging = GetForagingComponent();
    return configuration.markerMaxIntensity * std::pow(1.0f - configuration.markerDecayRate, 4.0f * foraging.walkTime);
}

float AntView::GetEnemyMarkerIntensity(const AntConfiguration &configuration) const {
    if (GetEncounterComponent().enemyTimer < 0.0f) {
        return 0.0f;
    }

    return configuration.markerMaxIntensity *
           std::pow(1.0f - configuration.markerDecayRate, 4.0f * GetEncounterComponent().enemyTimer);
}

bool AntView::IsDead() const { return GetEnergyComponent().IsDepleted(); }

void AntView::Kill() { GetEnergyComponent().Deplete(); }

void AntView::ConsumeEnergy(const float amount) {
    if (amount <= 0.0f) {
        return;
    }

    GetEnergyComponent().Consume(amount);
}

void AntView::RefillEnergy(const AntConfiguration &configuration) {
    GetEnergyComponent().Refill(configuration.antMaxEnergy);
}

bool AntView::IsCarryingFood() const {
    auto &foraging = GetForagingComponent();
    return foraging.state == ForagingState::ToHomeWithFood;
}

float AntView::GetMass() const { return IsCarryingFood() ? FoodMass : BaseMass; }

void AntView::BeginEncounter(const AntId opponent) {
    GetEncounterComponent().opponentId = opponent;
    GetEncounterComponent().enemyTimer = 0.0f;
}

void AntView::EndEncounter() {
    GetEncounterComponent().opponentId.reset();
    GetEncounterComponent().enemyTimer = -1.0f;
}

bool AntView::IsInEncounter() const { return GetEncounterComponent().opponentId.has_value(); }

float AntView::GetAngle() const { return Pose().direction.GetAngle(); }

pipeframe::Vector2f AntView::GetDirection() const { return Pose().direction.GetDirection(); }

AntId AntView::GetId() const { return ComponentView::GetEntity(); }

ColonyId AntView::GetColonyId() const { return Identity().colonyId; }

AntRole AntView::GetRole() const { return Identity().role; }

ForagingState AntView::GetState() const {
    auto &foraging = GetForagingComponent();
    return foraging.state;
}

pipeframe::Vector2f AntView::GetPosition() const { return Transform().position; }

pipeframe::Vector2f AntView::GetVelocity() const { return GetDirection() * Motion().speed; }

pipeframe::Vector2f AntView::GetTarget() const {
    auto &foraging = GetForagingComponent();
    return foraging.target;
}

float AntView::GetDistanceToTarget() const {
    auto &foraging = GetForagingComponent();
    return foraging.distanceToTarget;
}

float AntView::GetEnergy() const { return GetEnergyComponent().current; }

const std::array<AntLegPose, 6> &AntView::GetLegs() const { return Pose().legs; }

std::array<AntLegPose, 6> &AntView::GetLegs() { return Pose().legs; }

void AntView::CreateLegs() { InitializeAntLegs(Pose(), Transform()); }

pipeframe::Vector2f AntView::Normalize(const pipeframe::Vector2f value) {
    const float lengthSquared = value.x * value.x + value.y * value.y;

    if (lengthSquared <= 0.000001f) {
        return {
            1.0f,
            0.0f,
        };
    }

    return value / std::sqrt(lengthSquared);
}

float AntView::Distance(const pipeframe::Vector2f first, const pipeframe::Vector2f second) {
    const pipeframe::Vector2f difference = first - second;

    return std::sqrt(difference.x * difference.x + difference.y * difference.y);
}

} // namespace ant_simulation
