#ifndef ANT_MOVEMENT_SYSTEM_H
#define ANT_MOVEMENT_SYSTEM_H

#include "World/Physics/AntAvoidanceSystem.h"
#include "World/Physics/AntBodySystem.h"
#include "World/Physics/ContactSolver.h"
#include "World/Runtime/AntQuery.h"
#include <PipeFrame/Core/ThreadPool.h>
#include <PipeFrame/Simulation/System.h>
#include <cstddef>
#include <memory>

namespace ant_simulation {
struct AntMovementResult {
    std::size_t antContacts{}, wallConstraints{}, futureCollisions{};
    float preparationTimeMs{}, avoidanceTimeMs{}, physicsTimeMs{};
};

class AntMovementSystem final : public pipeframe::FixedUpdateSystem<AntMovementResult> {
  public:
    AntMovementSystem(AntQuery &, AntBodySystem &, ContactSolver &, const AntConfiguration &);
    [[nodiscard]] std::string_view GetSystemId() const override { return "ant.movement"; }
    AntMovementResult Update(float deltaTime) override;

  private:
    void AdvanceState(float deltaTime);
    void WriteMovementCommand(AntView &);
    void ReadMovementResult(AntView &, ForagingComponent &, float deltaTime) const;
    static pipeframe::Vector2f Normalize(pipeframe::Vector2f);
    AntQuery &store;
    AntBodySystem &physics;
    ContactSolver &contacts;
    const AntConfiguration &configuration;
    std::unordered_map<ColonyId, float> colonySpeeds;
    AntAvoidanceSystem avoidance;
    std::unique_ptr<pipeframe::ThreadPool> workers;
};
} // namespace ant_simulation
#endif
