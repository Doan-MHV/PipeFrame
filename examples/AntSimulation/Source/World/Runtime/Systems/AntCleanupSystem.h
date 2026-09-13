#ifndef ANT_CLEANUP_SYSTEM_H
#define ANT_CLEANUP_SYSTEM_H
#include "World/Physics/AntBodySystem.h"
#include "World/Physics/AntContactSystem.h"
#include "World/Runtime/AntQuery.h"
#include "World/Runtime/Systems/ColonyLifecycleSystem.h"
#include <PipeFrame/Simulation/System.h>
namespace ant_simulation {
struct AntCleanupResult {
    std::size_t enemyAlerts{}, removedAnts{};
};
class AntCleanupSystem final : public pipeframe::FixedUpdateSystem<AntCleanupResult> {
  public:
    AntCleanupSystem(AntQuery &, ColonyLifecycleSystem &, AntBodySystem &, pipeframe::Vector2i worldSize);
    [[nodiscard]] std::string_view GetSystemId() const override { return "ant.cleanup"; }
    AntCleanupResult Update(float) override;

  private:
    AntQuery &store;
    ColonyLifecycleSystem &colonies;
    AntBodySystem &physics;
    AntContactSystem contacts;
};
} // namespace ant_simulation
#endif
