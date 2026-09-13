#ifndef ANT_WORLD_CELL_H
#define ANT_WORLD_CELL_H

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "World/Runtime/Environment/Marker.h"

namespace ant_simulation {

using WorldEntityId = std::uint64_t;

inline constexpr WorldEntityId InvalidWorldEntityId{0};

enum class WorldCellEditState : std::uint8_t {
    None = 0,
    WallRequested = 2,
    EraseRequested = 4,
};

struct AntWorldCell {
    std::array<Marker, MarkerChannelCount> markers{};

    std::size_t foodQuantity{0};

    bool wall{false};

    WorldCellEditState editState{
        WorldCellEditState::None
    };

    WorldEntityId foodEntityId{
        InvalidWorldEntityId
    };

    WorldEntityId physicsObjectId{
        InvalidWorldEntityId
    };

    float markerSamplingCoefficient{1.0f};

    [[nodiscard]]
    Marker &GetMarker(const MarkerKind kind) {
        assert(IsMarkerChannel(kind));

        return markers[GetMarkerChannelIndex(kind)];
    }

    [[nodiscard]]
    const Marker &GetMarker(
        const MarkerKind kind
    ) const {
        assert(IsMarkerChannel(kind));

        return markers[GetMarkerChannelIndex(kind)];
    }

    void DecayMarkers(
        const float rate,
        const float deltaTime
    ) {
        for (Marker &marker : markers) {
            marker.Decay(rate, deltaTime);
        }
    }

    void AddMarker(
        MarkerKind kind,
        float intensity,
        ColonyId colonyId
    ) {
        if (!IsMarkerChannel(kind) ||
            colonyId == InvalidColonyId ||
            intensity <= 0.0f) {
            return;
        }

        Marker &marker = GetMarker(kind);

        if (marker.persistent) {
            return;
        }

        if (marker.colonyId != colonyId) {
            if (marker.intensity <= 0.0f) {
                marker.intensity = intensity;
                marker.colonyId = colonyId;
            } else {
                marker.intensity -= intensity;
            }

            return;
        }

        marker.intensity += intensity;
    }

    void SetPersistentMarker(
        MarkerKind kind,
        float intensity,
        ColonyId colonyId
    ) {
        if (!IsMarkerChannel(kind) ||
            colonyId == InvalidColonyId) {
            return;
        }

        Marker &marker = GetMarker(kind);

        marker.colonyId = colonyId;
        marker.intensity = intensity;
        marker.persistent = true;
    }

    [[nodiscard]]
    bool HasNavigationMarkers() const {
        return GetMarker(MarkerKind::ToHome).HasOwner() ||
               GetMarker(MarkerKind::ToFood).HasOwner();
    }

    [[nodiscard]]
    bool IsEmpty() const {
        return !HasNavigationMarkers();
    }

    [[nodiscard]]
    bool IsEraseRequested() const {
        return editState ==
               WorldCellEditState::EraseRequested;
    }

    [[nodiscard]]
    bool IsWallRequested() const {
        return editState ==
               WorldCellEditState::WallRequested;
    }

    void RequestErase() {
        editState = WorldCellEditState::EraseRequested;
    }

    void RequestWall() {
        editState = WorldCellEditState::WallRequested;
    }

    void ResetEditState() {
        editState = WorldCellEditState::None;
    }

    void ClearFood() {
        foodQuantity = 0;
        foodEntityId = InvalidWorldEntityId;
    }

    void ClearWall() {
        wall = false;
        physicsObjectId = InvalidWorldEntityId;
        markerSamplingCoefficient = 1.0f;
    }
};

} // namespace ant_simulation

#endif