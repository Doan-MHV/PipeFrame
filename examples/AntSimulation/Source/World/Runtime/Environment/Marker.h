#ifndef ANT_MARKER_H
#define ANT_MARKER_H

#include <cstddef>
#include <cstdint>

#include <PipeFrame/Project/ProjectTypes.h>

namespace ant_simulation {

using ColonyId = pipeframe::SceneObjectId;

inline constexpr ColonyId InvalidColonyId{0};

enum class MarkerKind : std::uint8_t {
    ToHome = 0,
    ToFood = 1,
    ToEnemy = 2,
    None = 3,
};

inline constexpr std::size_t MarkerChannelCount{3};

[[nodiscard]]
constexpr bool IsMarkerChannel(const MarkerKind kind) {
    return kind == MarkerKind::ToHome ||
           kind == MarkerKind::ToFood ||
           kind == MarkerKind::ToEnemy;
}

[[nodiscard]]
constexpr std::size_t GetMarkerChannelIndex(
    const MarkerKind kind
) {
    return static_cast<std::size_t>(kind);
}

struct Marker {
    ColonyId colonyId{InvalidColonyId};
    float intensity{0.0f};
    bool persistent{false};

    Marker() = default;

    Marker(
        ColonyId sourceColonyId,
        float initialIntensity,
        bool isPersistent = false
    );

    [[nodiscard]]
    bool HasOwner() const;

    [[nodiscard]]
    bool IsActive(float threshold = 0.0f) const;

    void Decay(float rate, float deltaTime);

    void Clear();
};

} // namespace ant_simulation

#endif