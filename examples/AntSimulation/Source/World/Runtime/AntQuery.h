#ifndef ANT_STORE_H
#define ANT_STORE_H

#include <cstddef>
#include <PipeFrame/ECS/SceneViewCache.h>

#include "World/Runtime/AntView.h"
#include "Components/AntIdentityComponent.h"
#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/Marker.h"

namespace ant_simulation {

class AntQuery final : public pipeframe::SceneViewCache<AntView, AntIdentityComponent> {
public:
    using SceneViewCache::SceneViewCache;
    AntView &Create(
        ColonyId colonyId,
        AntRole role,
        pipeframe::Vector2f position,
        float initialAngle,
        float initialMarkerOffset,
        const AntConfiguration &configuration
    );

    [[nodiscard]]
    AntView *Find(AntId id);

    [[nodiscard]]
    const AntView *Find(AntId id) const;

    [[nodiscard]]
    std::span<AntView> GetAnts();

    [[nodiscard]]
    std::span<const AntView> GetAnts() const;

    [[nodiscard]]
    std::size_t GetCount() const;

    [[nodiscard]]
    std::size_t RemoveDead();

    [[nodiscard]]
    std::size_t RemoveColony(ColonyId colonyId);

    void Clear();

};

} // namespace ant_simulation

#endif
