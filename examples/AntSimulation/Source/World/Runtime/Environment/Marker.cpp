#include "World/Runtime/Environment/Marker.h"

namespace ant_simulation {

Marker::Marker(const ColonyId sourceColonyId, const float initialIntensity, const bool isPersistent)
    : colonyId(sourceColonyId), intensity(initialIntensity), persistent(isPersistent) {}

bool Marker::HasOwner() const { return colonyId != InvalidColonyId; }

bool Marker::IsActive(const float threshold) const { return HasOwner() && intensity > threshold; }

void Marker::Decay(const float rate, const float deltaTime) {
    if (persistent) {
        return;
    }

    intensity *= 1.0f - rate * deltaTime;
}

void Marker::Clear() {
    colonyId = InvalidColonyId;
    intensity = 0.0f;
    persistent = false;
}

} // namespace ant_simulation