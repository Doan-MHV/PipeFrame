#include "Editor/ColonyInspector.h"

#include <algorithm>
#include <cmath>

namespace ant_simulation {

void ColonyInspector::SetSelectedColony(const std::optional<ColonyId> colonyId) {
    selectedColonyId = colonyId;

    if (!selectedColonyId.has_value()) {
        ClearData();
    }
}

void ColonyInspector::Refresh(const std::span<const ColonyView> colonies) {
    const ColonyView *colony = FindSelectedColony(colonies);

    if (colony == nullptr) {
        ClearData();
        return;
    }

    data.available = true;

    data.id = colony->GetId();

    data.position = colony->GetPosition();

    data.color = colony->GetColor();

    data.radius = colony->GetRadius();

    data.reserve = colony->GetReserve();

    data.foodQuantity = colony->GetFoodQuantity();

    data.collectionRate = colony->GetCollectionRate();

    data.antCount = colony->GetAntCount();

    data.soldierRequested = colony->State().soldierRequested;
}

bool ColonyInspector::SetPosition(const std::span<ColonyView> colonies, const pipeframe::Vector2f position) {
    if (!std::isfinite(position.x) || !std::isfinite(position.y)) {
        return false;
    }

    ColonyView *colony = FindSelectedColony(colonies);

    if (colony == nullptr) {
        return false;
    }

    colony->Transform().position = position;

    Refresh(colonies);

    return true;
}

bool ColonyInspector::SetRadius(const std::span<ColonyView> colonies, const float radius) {
    if (!std::isfinite(radius) || radius < 0.0f) {
        return false;
    }

    ColonyView *colony = FindSelectedColony(colonies);

    if (colony == nullptr) {
        return false;
    }

    colony->SetRadius(radius);

    Refresh(colonies);

    return true;
}

bool ColonyInspector::SetReserve(const std::span<ColonyView> colonies, const float reserve) {
    if (!std::isfinite(reserve) || reserve < 0.0f) {
        return false;
    }

    ColonyView *colony = FindSelectedColony(colonies);

    if (colony == nullptr) {
        return false;
    }

    colony->SetReserve(reserve);

    Refresh(colonies);

    return true;
}

bool ColonyInspector::SetSoldierRequested(const std::span<ColonyView> colonies, const float soldierRequested) {
    if (!std::isfinite(soldierRequested) || soldierRequested < 0.0f) {
        return false;
    }

    ColonyView *colony = FindSelectedColony(colonies);

    if (colony == nullptr) {
        return false;
    }

    colony->State().soldierRequested = soldierRequested;

    Refresh(colonies);

    return true;
}

bool ColonyInspector::SetColor(const std::span<ColonyView> colonies, const pipeframe::Color color) {
    ColonyView *colony = FindSelectedColony(colonies);

    if (colony == nullptr) {
        return false;
    }

    colony->State().color = color;

    Refresh(colonies);

    return true;
}

std::optional<ColonyId> ColonyInspector::GetSelectedColony() const { return selectedColonyId; }

const ColonyInspectorData &ColonyInspector::GetData() const { return data; }

ColonyView *ColonyInspector::FindSelectedColony(const std::span<ColonyView> colonies) const {
    if (!selectedColonyId.has_value()) {
        return nullptr;
    }

    const auto iterator = std::find_if(colonies.begin(), colonies.end(), [this](const ColonyView &colony) {
        return colony.GetId() == *selectedColonyId;
    });

    if (iterator == colonies.end()) {
        return nullptr;
    }

    return &*iterator;
}

const ColonyView *ColonyInspector::FindSelectedColony(const std::span<const ColonyView> colonies) const {
    if (!selectedColonyId.has_value()) {
        return nullptr;
    }

    const auto iterator = std::find_if(colonies.begin(), colonies.end(), [this](const ColonyView &colony) {
        return colony.GetId() == *selectedColonyId;
    });

    if (iterator == colonies.end()) {
        return nullptr;
    }

    return &*iterator;
}

void ColonyInspector::ClearData() { data = {}; }

} // namespace ant_simulation