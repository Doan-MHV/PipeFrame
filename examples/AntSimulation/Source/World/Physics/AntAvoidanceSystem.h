#ifndef ANT_AVOIDANCE_SYSTEM_H
#define ANT_AVOIDANCE_SYSTEM_H

#include <PipeFrame/Simulation/System.h>
#include <cstddef>
#include <optional>
#include <vector>

#include "Configuration/AntConfiguration.h"
#include "World/Physics/AntBodySystem.h"
#include "World/Runtime/AntQuery.h"
#include <PipeFrame/Spatial/UniformSpatialIndex.h>

namespace ant_simulation {

class AntAvoidanceSystem final : public pipeframe::FixedUpdateSystem<std::size_t> {
  public:
    static constexpr float MaximumTimeToCollision{2.0f};

    static constexpr std::size_t SliceCount{2};

    struct FutureCollision {
        PhysicsBodyId firstBodyId{InvalidPhysicsBodyId};

        PhysicsBodyId secondBodyId{InvalidPhysicsBodyId};

        float timeToCollision{-1.0f};
    };

    AntAvoidanceSystem(AntQuery &antStore, AntBodySystem &physicsWorld, const AntConfiguration &configuration);

    [[nodiscard]] std::string_view GetSystemId() const override { return "ant.collision-avoidance"; }
    std::size_t Update(float deltaTime) override;

    [[nodiscard]]
    const std::vector<FutureCollision> &GetFutureCollisions() const;

    [[nodiscard]]
    static std::optional<float> CalculateTimeToCollision(const AntPhysicsBody &first, const AntPhysicsBody &second,
                                                         float combinedRadius = 1.0f);

  private:
    void FindFutureCollisions();

    void SolveFutureCollisions(float deltaTime);

    AntQuery &antStore;
    AntBodySystem &physicsWorld;

    const AntConfiguration &configuration;

    // Body pointers are borrowed only until this Update completes. Body storage
    // cannot change while the physics batch is running.
    pipeframe::UniformSpatialIndex<AntPhysicsBody *> collisionGrid;
    std::vector<pipeframe::UniformSpatialIndex<AntPhysicsBody *>::Entry> gridEntries;

    std::vector<FutureCollision> futureCollisions;

    std::size_t currentSlice{0};
};

} // namespace ant_simulation

#endif
