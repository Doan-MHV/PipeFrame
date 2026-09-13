#include "World/Runtime/AntQuery.h"
#include "Runtime/AntRegistration.h"

#include <cstddef>

namespace ant_simulation {

AntView &AntQuery::Create(
    const ColonyId colonyId,
    const AntRole role,
    const pipeframe::Vector2f position,
    const float initialAngle,
    const float initialMarkerOffset,
    const AntConfiguration &configuration
) {
    const AntEntity recipe(colonyId, role, position, initialAngle, initialMarkerOffset, configuration);
    Refresh();
    return Track(AntEntityTypes().Spawn(GetScene(),recipe));
}

AntView *AntQuery::Find(
    const AntId id
) {
    return SceneViewCache::Find(id);
}

const AntView *AntQuery::Find(
    const AntId id
) const {
    return SceneViewCache::Find(id);
}

std::span<AntView> AntQuery::GetAnts() {
    return Items();
}

std::span<const AntView>
AntQuery::GetAnts() const {
    return Items();
}

std::size_t AntQuery::GetCount() const {
    return Size();
}

std::size_t AntQuery::RemoveDead() {
    return RemoveWhere([](const AntView &ant){return ant.IsDead();});
}

std::size_t AntQuery::RemoveColony(
    const ColonyId colonyId
) {
    return RemoveWhere([colonyId](const AntView &ant){return ant.GetColonyId()==colonyId;});
}

void AntQuery::Clear() {
    SceneViewCache::Clear();
}

} // namespace ant_simulation
