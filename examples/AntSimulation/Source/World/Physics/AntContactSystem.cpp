#include "World/Physics/AntContactSystem.h"

#include "Components/AntIdentityComponent.h"
#include "World/Runtime/AntView.h"

namespace ant_simulation {

AntContactSystem::AntContactSystem(AntQuery &newAntQuery, const pipeframe::Vector2i worldSize) : antStore(newAntQuery) {
    collisionGrid.Initialize(
        {
            static_cast<float>(worldSize.x),
            static_cast<float>(worldSize.y),
        },
        ContactDistance);
}

std::size_t AntContactSystem::ProcessContacts() {
    collisionGrid.Rebuild(antStore.GetAnts());

    std::size_t alertedAntCount{0};
    const auto encounters = antStore.GetWorld().BorrowComponents<AntEncounterComponent>();

    for (AntView &ant : antStore.GetAnts()) {
        if (ant.IsDead()) {
            continue;
        }

        // This intentionally matches AntPezza.
        // Its soldier contact implementation is currently empty.
        if (ant.GetRole() == AntRole::Soldier) {
            continue;
        }

        if (!HasEnemyContact(ant)) {
            continue;
        }

        auto *encounter = encounters.Get(ant.GetId());
        if (!encounter)
            continue;
        encounter->enemyTimer = 0.0f;
        ++alertedAntCount;
    }

    return alertedAntCount;
}

bool AntContactSystem::HasEnemyContact(const AntView &ant) const {
    const CollisionGrid::CellRange range =
        collisionGrid.GetCellsOverlapping({ant.GetPosition().x, ant.GetPosition().y}, ContactDistance);

    if (range.IsEmpty()) {
        return false;
    }

    const float contactDistanceSquared = ContactDistance * ContactDistance;

    for (int row = range.minimumRow; row <= range.maximumRow; ++row) {
        for (int column = range.minimumColumn; column <= range.maximumColumn; ++column) {
            for (const AntId candidateId : collisionGrid.GetAntIds(column, row)) {
                if (candidateId == ant.GetId()) {
                    continue;
                }

                const AntView *candidate = antStore.Find(candidateId);

                if (candidate == nullptr || candidate->IsDead() || candidate->GetColonyId() == ant.GetColonyId()) {
                    continue;
                }

                const pipeframe::Vector2f difference{candidate->GetPosition().x - ant.GetPosition().x,
                                                     candidate->GetPosition().y - ant.GetPosition().y};

                const float distanceSquared = difference.x * difference.x + difference.y * difference.y;

                if (distanceSquared <= contactDistanceSquared) {
                    return true;
                }
            }
        }
    }

    return false;
}

} // namespace ant_simulation
