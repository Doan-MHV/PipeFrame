#ifndef ANT_PHYSICS_WORLD_H
#define ANT_PHYSICS_WORLD_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

#include <PipeFrame/Physics/BodyStorage.h>
#include <PipeFrame/Simulation/System.h>

#include "Configuration/AntConfiguration.h"
#include "World/Runtime/AntQuery.h"
#include "World/Runtime/AntView.h"

namespace ant_simulation {

using PhysicsBodyId = pipeframe::PhysicsBodyId;
inline constexpr PhysicsBodyId InvalidPhysicsBodyId = pipeframe::InvalidPhysicsBodyId;

struct AntPhysicsBody : pipeframe::PhysicsBody2D {
    AntId antId{InvalidAntId};
    ColonyId colonyId{InvalidColonyId};
    pipeframe::Vector2f direction{1.0f, 0.0f};
    AntPhysicsBody() {
        mass = AntView::BaseMass;
        response = 0.04f;
        radius = 0.5f;
    }
};

class AntBodySystem final : public pipeframe::FixedUpdateSystem<void> {
  public:
    AntBodySystem(AntQuery &antStore, const AntConfiguration &configuration);

    void Synchronize();

    void Step(float deltaTime);
    std::string_view GetSystemId() const override { return "ant.physics-bodies"; }
    void Update(float delta) override { Step(delta); }

    void SynchronizeAntPositions();

    [[nodiscard]]
    AntPhysicsBody *FindBody(PhysicsBodyId id);

    [[nodiscard]]
    const AntPhysicsBody *FindBody(PhysicsBodyId id) const;

    [[nodiscard]]
    AntPhysicsBody *FindAntBody(AntId antId);

    [[nodiscard]]
    const AntPhysicsBody *FindAntBody(AntId antId) const;

    bool SetVelocity(AntId antId, pipeframe::Vector2f velocity);

    bool SetDirection(AntId antId, pipeframe::Vector2f direction);

    bool Teleport(AntId antId, pipeframe::Vector2f position);

    [[nodiscard]]
    std::size_t GetBodyCount() const;

    [[nodiscard]]
    std::span<AntPhysicsBody> GetBodies();

    [[nodiscard]]
    std::span<const AntPhysicsBody> GetBodies() const;

    void Clear();

  private:
    AntPhysicsBody &CreateBody(AntView &ant);

    void RemoveBodyAt(std::size_t index);

    AntQuery &antStore;
    const AntConfiguration &configuration;

    pipeframe::BodyStorage<AntPhysicsBody> storage;

    std::unordered_map<AntId, PhysicsBodyId> antBodyIds;
};

} // namespace ant_simulation

#endif
