#ifndef ANT_WORLD_H
#define ANT_WORLD_H

#include <cstddef>
#include <PipeFrame/World/World.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <PipeFrame/Foundation/MathTypes.h>

#include "World/Runtime/AntStepResult.h"
#include "World/Physics/AntMovementSystem.h"
#include "World/Runtime/Systems/AntForagingSystem.h"
#include "World/Runtime/Systems/AntCleanupSystem.h"
#include "World/Runtime/AntQuery.h"
#include "World/Runtime/Systems/ColonyLifecycleSystem.h"
#include "Configuration/AntConfiguration.h"
#include "World/Physics/AntBodySystem.h"
#include "World/Runtime/Environment/AntEnvironment.h"

namespace ant_simulation {
class AntRuntimeWorld;
class AntPhysicsWorld;

struct AntSimulationWorldStatistics {
    std::uint64_t tick{0};

    std::size_t colonyCount{0};
    std::size_t antCount{0};
    std::size_t physicsBodyCount{0};

    std::size_t foodEntityCount{0};
    std::size_t totalFoodQuantity{0};
    std::size_t wallCount{0};

    std::uint64_t totalBirths{0};
    std::uint64_t totalDeaths{0};

    AntSimulationStepResult lastAntUpdate;
};

struct AntColonyCheckpoint {
    ColonyId id{InvalidColonyId};
    std::size_t antCount{0};
    float reserve{0.0f};
    float foodQuantity{0.0f};
    float collectionRate{0.0f};
};

struct AntMarkerCheckpoint {
    ColonyId colonyId{InvalidColonyId};
    std::size_t ownedCellCount{0};
    double totalIntensity{0.0};
};

struct AntBehaviorCheckpoint {
    std::uint64_t tick{0};
    std::uint64_t stateSignature{0};
    std::uint64_t totalBirths{0};
    std::uint64_t totalDeaths{0};
    std::size_t antCount{0};
    std::size_t physicsBodyCount{0};
    std::size_t totalFoodQuantity{0};
    std::vector<AntColonyCheckpoint> colonies;
    std::vector<AntMarkerCheckpoint> markers;
};

class AntWorld final : public pipeframe::World {
public:
    explicit AntWorld(
        AntConfiguration configuration,
        std::uint32_t randomSeed = 0
    );

    ~AntWorld();

    AntWorld(
        const AntWorld &
    ) = delete;

    AntWorld &operator=(
        const AntWorld &
    ) = delete;

    bool Initialize(
        std::string &errorMessage
    );

    bool Reset(
        std::string &errorMessage
    );

    void Clear();

    ColonyView &CreateColony(
        ColonyId id,
        pipeframe::Vector2f position,
        pipeframe::Color color
    );

    [[nodiscard]]
    bool RemoveColony(ColonyId id);

    ColonyView &CreateColony(
        pipeframe::Vector2f position,
        pipeframe::Color color
    );

    AntSimulationStepResult FixedUpdate(
        float fixedDeltaTime
    );

    void BeginFixedStep(float fixedDeltaTime);
    void UpdateMovement(float fixedDeltaTime);
    void UpdateBehavior(float fixedDeltaTime);
    void EndFixedStep(float fixedDeltaTime);

    [[nodiscard]]
    bool IsInitialized() const;

    [[nodiscard]]
    const AntConfiguration &
    GetConfiguration() const;

    [[nodiscard]]
    AntEnvironment &GetEnvironment();

    [[nodiscard]]
    const AntEnvironment &
    GetEnvironment() const;

    [[nodiscard]]
    pipeframe::BehaviourScene &GetScene();

    AntQuery &GetAntQuery();

    [[nodiscard]]
    const AntQuery &GetAntQuery() const;

    [[nodiscard]]
    ColonyLifecycleSystem &GetColonyLifecycleSystem();

    [[nodiscard]]
    const ColonyLifecycleSystem &
    GetColonyLifecycleSystem() const;

    [[nodiscard]]
    AntBodySystem &GetPhysicsBodies();

    AntPhysicsWorld &GetPhysicsWorld();
    AntRuntimeWorld &GetRuntimeWorld();

    [[nodiscard]]
    const AntBodySystem &
    GetPhysicsBodies() const;

    [[nodiscard]]
    const AntSimulationWorldStatistics &
    GetStatistics() const;

    [[nodiscard]]
    AntBehaviorCheckpoint CaptureBehaviorCheckpoint() const;

private:
    struct State;

    void RefreshStatistics();

    AntConfiguration configuration;
    std::uint32_t randomSeed{0};

    std::unique_ptr<State> state;

    AntSimulationWorldStatistics statistics;
};

} // namespace ant_simulation

#endif
