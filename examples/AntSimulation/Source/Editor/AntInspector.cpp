#include "Editor/AntInspector.h"

#include <algorithm>
#include <string>

namespace ant_simulation {

AntInspector::AntInspector(const AntConfiguration &newConfiguration) : configuration(newConfiguration) {}

void AntInspector::SetSelectedAnt(const std::optional<AntId> antId) {
    selectedAntId = antId;

    if (!selectedAntId.has_value()) {
        ClearData();
    }
}

void AntInspector::SetFollowEnabled(const bool enabled) {
    followEnabled = enabled;
    data.follow = enabled;
}

void AntInspector::SetHighlightEnabled(const bool enabled) {
    highlightEnabled = enabled;
    data.highlight = enabled;
}

void AntInspector::SetShowTargetEnabled(const bool enabled) {
    showTargetEnabled = enabled;
    data.showTarget = enabled;
}

void AntInspector::Refresh(const std::span<const AntView> ants) {
    const AntView *ant = FindSelectedAnt(ants);

    if (ant == nullptr) {
        ClearData();
        return;
    }

    data.available = true;

    data.id = ant->GetId();
    data.colonyId = ant->GetColonyId();

    data.displayName =
        "Ant " + std::to_string(ant->Identity().nameIndex + 1) + "-" + std::to_string(ant->Identity().nameSuffix);

    data.role = std::string{ToString(ant->GetRole())};

    data.state = std::string{ToString(ant->GetState())};

    data.position = ant->GetPosition();

    data.target = ant->GetTarget();

    data.color = ant->Identity().color;

    const float antAngle = ant->GetAngle();

    const float cosine = std::cos(antAngle);

    const float sine = std::sin(antAngle);

    const pipeframe::Vector2f antPosition = ant->GetPosition();

    const auto worldToAntLocal = [antPosition, cosine, sine](const pipeframe::Vector2f worldPosition) {
        const pipeframe::Vector2f difference = worldPosition - antPosition;

        return pipeframe::Vector2f{
            difference.x * cosine + difference.y * sine,

            -difference.x * sine + difference.y * cosine,
        };
    };

    const std::array<AntLegPose, 6> &antLegs = ant->GetLegs();

    for (std::size_t index = 0; index < antLegs.size(); ++index) {

        data.legs[index].start = antLegs[index].GetRelativeStart();

        data.legs[index].end = worldToAntLocal(antLegs[index].GetCurrentWorldEnd());
    }

    data.energy = ant->GetEnergy();

    data.energyRatio = configuration.antMaxEnergy > 0.0f
                           ? std::clamp(ant->GetEnergy() / configuration.antMaxEnergy, 0.0f, 1.0f)
                           : 0.0f;

    data.speed = ant->Motion().speed;

    data.speedRatio =
        configuration.antSpeed > 0.0f ? std::clamp(ant->Motion().speed / configuration.antSpeed, 0.0f, 1.0f) : 0.0f;

    data.blockedRatio = std::clamp(ant->GetBlockedRatio(configuration), 0.0f, 1.0f);

    data.distanceToTarget = ant->GetDistanceToTarget();

    data.totalTravelDistance = ant->Motion().travelDistance;

    data.collectedFood = ant->GetForagingComponent().collectedFood;

    data.carryingFood = ant->IsCarryingFood();

    data.inEncounter = ant->IsInEncounter();

    data.dead = ant->IsDead();

    data.follow = followEnabled;
    data.highlight = highlightEnabled;
    data.showTarget = showTargetEnabled;
}

std::optional<AntId> AntInspector::GetSelectedAnt() const { return selectedAntId; }

const AntInspectorData &AntInspector::GetData() const { return data; }

const AntView *AntInspector::FindSelectedAnt(const std::span<const AntView> ants) const {
    if (!selectedAntId.has_value()) {
        return nullptr;
    }

    const auto iterator =
        std::find_if(ants.begin(), ants.end(), [this](const AntView &ant) { return ant.GetId() == *selectedAntId; });

    if (iterator == ants.end()) {
        return nullptr;
    }

    return &*iterator;
}

void AntInspector::ClearData() {
    data = {};

    data.follow = followEnabled;
    data.highlight = highlightEnabled;
    data.showTarget = showTargetEnabled;
}

} // namespace ant_simulation