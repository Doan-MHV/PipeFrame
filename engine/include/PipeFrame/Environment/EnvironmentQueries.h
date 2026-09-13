#pragma once
#include <PipeFrame/Environment/TilemapQueries.h>
namespace pipeframe {
// Scene-local query service. Behaviours query authored geometry without owning a map
// or depending on a project's concrete runtime. Return values are snapshots.
class EnvironmentQueries {
public:
    virtual ~EnvironmentQueries() = default;
    virtual std::optional<TilemapQueryHit> RaycastEnvironment(Vector2f origin,Vector2f direction,float distance,std::uint32_t mask=~std::uint32_t{})=0;
    virtual std::vector<TilemapQueryHit> OverlapEnvironment(Circle2D circle,std::uint32_t mask=~std::uint32_t{})=0;
    virtual bool HasEnvironmentClearance(Circle2D circle,std::uint32_t mask=~std::uint32_t{})=0;
    virtual std::optional<TilemapQueryHit> SweepEnvironment(Circle2D circle,Vector2f displacement,std::uint32_t mask=~std::uint32_t{},std::uint64_t ignoredObject=0,bool ignoreMoving=false)=0;
};
}
