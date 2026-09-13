#include "World/Runtime/Systems/WorkerBehavior.h"

#include "World/Runtime/AntView.h"
#include "World/Runtime/ColonyView.h"
#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/Marker.h"

namespace ant_simulation {

WorkerBehavior::WorkerBehavior(
    AntEnvironment &newEnvironment,
    const AntConfiguration &newConfiguration
)
    : environment(newEnvironment),
      configuration(newConfiguration),
      sampler(
          newEnvironment,
          newConfiguration) {
}

void WorkerBehavior::Update(
    AntView &ant,
    ForagingComponent &foraging,
    ColonyView &colony,
    const float deltaTime,
    std::mt19937 &randomGenerator
) const {
    // WorkerBehavior does not advance time itself.
    // The registered movement system advances timers before foraging.
    (void)deltaTime;

    AntWorldCell *cell =
        environment.TryGetCellAtWorldPosition(
            ant.GetPosition());

    if (cell == nullptr) {
        ant.Kill();
        return;
    }

    if (cell->wall) {
        ant.Kill();
    } else if (
        cell->foodQuantity > 0 &&
        ant.GetMarkerFocus() ==
            MarkerKind::ToFood) {
        CollectFood(ant, foraging);
    }

    const bool blocked =
        ant.IsBlocked(configuration);

    if (ant.IsTargetReached() || blocked) {
        foraging.blocked = blocked;

        sampler.SampleWorldIntensity(
            ant,
            randomGenerator);
    }

    CheckDistanceToColony(
        ant,
        foraging,
        colony);

    UpdateAntMarker(
        ant,
        foraging,
        *cell);
}

void WorkerBehavior::CollectFood(
    AntView &ant,
    ForagingComponent &foraging
) const {
    const std::size_t consumed =
        environment.ConsumeFood(
            ant.GetPosition(),
            1);

    if (consumed == 0) {
        return;
    }

    foraging.state = ForagingState::ToHomeWithFood;

    ant.RefillEnergy(configuration);
    foraging.walkTime = 0.0f;
}

void WorkerBehavior::CheckDistanceToColony(
    AntView &ant,
    ForagingComponent &foraging,
    ColonyView &colony
) const {
    const pipeframe::Vector2f difference =
        ant.GetPosition() -
        colony.GetPosition();

    const float distanceSquared =
        difference.x * difference.x +
        difference.y * difference.y;

    const float colonyRadiusSquared =
        colony.GetRadius() *
        colony.GetRadius();

    if (distanceSquared >=
        colonyRadiusSquared) {
        return;
    }

    foraging.walkTime = 0.0f;

    if (foraging.state ==
        ForagingState::ToHomeWithFood) {
        ++foraging.collectedFood;

        foraging.state = ForagingState::ToFood;

        colony.AddFood(1.0f);
    } else if (
        foraging.state ==
        ForagingState::ToHomeNoFood) {
        foraging.state = ForagingState::ToFood;
    }

    ant.RefillEnergy(configuration);

    // This matches AntPezza's current colony behavior.
    colony.State().soldierRequested += 0.2f;
    ant.GetEncounterComponent().enemyTimer = 0.0f;
}

void WorkerBehavior::UpdateAntMarker(
    AntView &ant,
    ForagingComponent &foraging,
    AntWorldCell &cell
) const {
    if (!ant.IsMarkerReady(
            configuration.antMarkerDistance)) {
        return;
    }

    // DropMarker also resets the marker timer and position.
    // ToHomeNoFood returns None but must still reset them.
    const MarkerKind dropKind =
        ant.DropMarker();

    if (dropKind != MarkerKind::None) {
        cell.AddMarker(
            dropKind,
            ant.GetMarkerIntensity(
                configuration),
            ant.GetColonyId());
    }

    if (ant.GetEncounterComponent().enemyTimer >= 0.0f) {
        cell.AddMarker(
            MarkerKind::ToEnemy,
            ant.GetEnemyMarkerIntensity(
                configuration),
            ant.GetColonyId());
    }

    Marker &focusedMarker =
        cell.GetMarker(
            ant.GetMarkerFocus());

    if (!focusedMarker.persistent &&
        focusedMarker.colonyId ==
            ant.GetColonyId() &&
        foraging.blocked) {
        focusedMarker.intensity = 0.0f;
    }
}

} // namespace ant_simulation