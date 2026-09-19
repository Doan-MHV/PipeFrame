#include "World/Runtime/ColonyView.h"

#include <algorithm>

namespace ant_simulation {

void ColonyView::Initialize(ColonyId id, pipeframe::Vector2f position, pipeframe::Color color,
                            const AntConfiguration &configuration, std::size_t samples) {
    State().id = id;
    Transform().position = position;
    State().color = color;
    State().radius = configuration.colonyRadius;
    State().reserve = static_cast<float>(configuration.colonyInitialAntCount) * configuration.antCost;
    History().collectionWindowSamples = std::max<std::size_t>(2, samples);
}

void ColonyView::AddFood(const float quantity) {
    if (quantity <= 0.0f) {
        return;
    }

    State().foodQuantity += quantity;
    State().reserve += quantity;
}

void ColonyView::UpdateCollectionRate() {
    History().collectionHistory.push_back(State().foodQuantity);

    while (History().collectionHistory.size() > History().collectionWindowSamples) {
        History().collectionHistory.pop_front();
    }

    if (History().collectionHistory.size() < 2) {
        State().collectionRate = 0.0f;
        return;
    }

    State().collectionRate = History().collectionHistory.back() - History().collectionHistory.front();
}

std::size_t ColonyView::AcquireNameSuffix(const std::size_t nameIndex) {
    const auto iterator = History().nameAttribution.find(nameIndex);

    if (iterator != History().nameAttribution.end()) {
        const std::size_t suffix = iterator->second;

        ++iterator->second;

        return suffix + 1;
    }

    History().nameAttribution[nameIndex] = 1;

    return 1;
}

ColonyId ColonyView::GetId() const { return State().id; }

pipeframe::Vector2f ColonyView::GetPosition() const { return Transform().position; }

pipeframe::Color ColonyView::GetColor() const { return State().color; }

float ColonyView::GetRadius() const { return State().radius; }

float ColonyView::GetReserve() const { return State().reserve; }

float ColonyView::GetFoodQuantity() const { return State().foodQuantity; }

float ColonyView::GetCollectionRate() const { return State().collectionRate; }

std::size_t ColonyView::GetAntCount() const { return GetMemberCount(); }

void ColonyView::SetRadius(const float newRadius) { State().radius = std::max(0.0f, newRadius); }

void ColonyView::SetReserve(const float newReserve) { State().reserve = std::max(0.0f, newReserve); }

void ColonyView::SetAntCount(const std::size_t newAntCount) { SetMemberCount(newAntCount); }

} // namespace ant_simulation