#include "World/Runtime/AntRuntimeWorld.h"
#include "World/Physics/AntPhysicsWorld.h"
#include "World/AntWorld.h"

#include <cassert>
#include <chrono>
#include <PipeFrame/Core/FixedStepSequence.h>
#include <bit>
#include <map>
#include <type_traits>
#include <utility>

#include "World/Physics/ContactSolver.h"

namespace ant_simulation {

namespace {

void HashBytes(
    std::uint64_t &hash,
    const void *data,
    const std::size_t size
) {
    const auto *bytes = static_cast<const unsigned char *>(data);
    for (std::size_t index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= 1099511628211ULL;
    }
}

template <typename Value>
void HashValue(std::uint64_t &hash, const Value &value) {
    static_assert(std::is_trivially_copyable_v<Value>);
    HashBytes(hash, &value, sizeof(Value));
}

} // namespace

struct AntWorld::State {
    State(const AntConfiguration &configuration,std::uint32_t seed)
        :runtime(configuration,seed),physics(runtime.ants,runtime.environment,configuration) {
        runtime.BindPhysics(physics.bodies,configuration);
    }
    AntRuntimeWorld runtime;
    AntPhysicsWorld physics;
};

AntWorld::AntWorld(
    AntConfiguration newConfiguration,
    const std::uint32_t newRandomSeed
)
    : configuration(
          std::move(newConfiguration)),
      randomSeed(newRandomSeed) {
}

AntPhysicsWorld &AntWorld::GetPhysicsWorld(){assert(state);return state->physics;}
AntRuntimeWorld &AntWorld::GetRuntimeWorld(){assert(state);return state->runtime;}

AntWorld::~AntWorld() =
    default;

bool AntWorld::Initialize(
    std::string &errorMessage
) {
    std::unique_ptr<State> newState =
        std::make_unique<State>(
            configuration,
            randomSeed);

    if (!newState->runtime.environment.Initialize(
            configuration,
            errorMessage)) {
        state.reset();
        statistics = {};

        return false;
    }

    state = std::move(newState);
    ResetSchedule();
    statistics = {};

    RefreshStatistics();

    errorMessage.clear();

    return true;
}

bool AntWorld::Reset(
    std::string &errorMessage
) {
    return Initialize(errorMessage);
}

void AntWorld::Clear() {
    state.reset();
    statistics = {};
}

ColonyView &AntWorld::CreateColony(
    const ColonyId id,
    const pipeframe::Vector2f position,
    const pipeframe::Color color
) {
    assert(state != nullptr);

    ColonyView &colony =
        state->runtime.colonies.CreateColony(
            id,
            position,
            color);

    RefreshStatistics();

    return colony;
}

bool AntWorld::RemoveColony(
    const ColonyId id
) {
    if (state == nullptr ||
        state->runtime.colonies.FindColony(id) == nullptr) {
        return false;
    }

    const std::size_t removedAnts =
        state->runtime.colonies.RemoveColony(id);
    statistics.totalDeaths += removedAnts;
    state->physics.bodies.Synchronize();
    RefreshStatistics();

    return true;
}

ColonyView &AntWorld::CreateColony(
    const pipeframe::Vector2f position,
    const pipeframe::Color color
) {
    assert(state != nullptr);

    ColonyView &colony =
        state->runtime.colonies.CreateColony(
            position,
            color);

    RefreshStatistics();

    return colony;
}

AntSimulationStepResult
AntWorld::FixedUpdate(
    const float fixedDeltaTime
) {
    if (state == nullptr ||
        fixedDeltaTime <= 0.0f) {
        return statistics.lastAntUpdate;
    }

    BeginFixedStep(fixedDeltaTime);
    UpdateMovement(fixedDeltaTime);
    UpdateBehavior(fixedDeltaTime);
    EndFixedStep(fixedDeltaTime);

    return statistics.lastAntUpdate;
}

void AntWorld::BeginFixedStep(const float fixedDeltaTime) {
    if (!state || fixedDeltaTime <= 0.0f) return;
    RunPhase(0, fixedDeltaTime, [&] {
        const std::size_t before = state->runtime.ants.GetCount();
        state->runtime.Update(fixedDeltaTime);
        statistics.totalBirths += state->runtime.ants.GetCount() - before;
    });
}

void AntWorld::UpdateMovement(const float fixedDeltaTime) {
    if (!state || fixedDeltaTime <= 0.0f) return;
    RunPhase(1, fixedDeltaTime, [&] {
        state->physics.Update(fixedDeltaTime);
        const AntMovementResult movement = state->physics.lastResult;
        auto &result = statistics.lastAntUpdate;
        result.antContacts = movement.antContacts;
        result.wallConstraints = movement.wallConstraints;
        result.futureCollisions = movement.futureCollisions;
        result.preparationTimeMs = movement.preparationTimeMs;
        result.avoidanceTimeMs = movement.avoidanceTimeMs;
        result.physicsTimeMs = movement.physicsTimeMs;
    });
}

void AntWorld::UpdateBehavior(const float fixedDeltaTime) {
    if (!state || fixedDeltaTime <= 0.0f) return;
    RunPhase(2, fixedDeltaTime, [&] {
        const auto started = std::chrono::steady_clock::now();
        state->runtime.foraging.Update(fixedDeltaTime);
        statistics.lastAntUpdate.behaviorTimeMs = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - started).count();
    });
}

void AntWorld::EndFixedStep(const float fixedDeltaTime) {
    if (!state || fixedDeltaTime <= 0.0f) return;
    RunPhase(3, fixedDeltaTime, [&] {
        const auto started = std::chrono::steady_clock::now();
        const AntCleanupResult cleanup = state->runtime.cleanup->Update(fixedDeltaTime);
        auto &result = statistics.lastAntUpdate;
        result.cleanupTimeMs = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - started).count();
        result.enemyAlerts = cleanup.enemyAlerts;
        result.removedAnts = cleanup.removedAnts;
        result.livingAnts = state->runtime.ants.GetCount();
        statistics.totalDeaths += cleanup.removedAnts;
        ++statistics.tick;
        RefreshStatistics();
    });
}

bool AntWorld::IsInitialized() const {
    return
        state != nullptr &&
        state->runtime.environment.IsInitialized();
}

const AntConfiguration &
AntWorld::GetConfiguration() const {
    return configuration;
}

AntEnvironment &
AntWorld::GetEnvironment() {
    assert(state != nullptr);
    return state->runtime.environment;
}

const AntEnvironment &
AntWorld::GetEnvironment() const {
    assert(state != nullptr);
    return state->runtime.environment;
}

pipeframe::BehaviourScene &AntWorld::GetScene() {
    assert(state != nullptr);
    return state->runtime.GetScene();
}

AntQuery &AntWorld::GetAntQuery() {
    assert(state != nullptr);
    return state->runtime.ants;
}

const AntQuery &
AntWorld::GetAntQuery() const {
    assert(state != nullptr);
    return state->runtime.ants;
}

ColonyLifecycleSystem &
AntWorld::GetColonyLifecycleSystem() {
    assert(state != nullptr);
    return state->runtime.colonies;
}

const ColonyLifecycleSystem &
AntWorld::GetColonyLifecycleSystem() const {
    assert(state != nullptr);
    return state->runtime.colonies;
}

AntBodySystem &
AntWorld::GetPhysicsBodies() {
    assert(state != nullptr);
    return state->physics.bodies;
}

const AntBodySystem &
AntWorld::GetPhysicsBodies() const {
    assert(state != nullptr);
    return state->physics.bodies;
}





const AntSimulationWorldStatistics &
AntWorld::GetStatistics() const {
    return statistics;
}

AntBehaviorCheckpoint
AntWorld::CaptureBehaviorCheckpoint() const {
    AntBehaviorCheckpoint checkpoint;

    if (state == nullptr) {
        return checkpoint;
    }

    checkpoint.tick = statistics.tick;
    checkpoint.totalBirths = statistics.totalBirths;
    checkpoint.totalDeaths = statistics.totalDeaths;
    checkpoint.antCount = statistics.antCount;
    checkpoint.physicsBodyCount = statistics.physicsBodyCount;
    checkpoint.totalFoodQuantity = statistics.totalFoodQuantity;

    std::uint64_t hash{14695981039346656037ULL};
    HashValue(hash, checkpoint.tick);
    HashValue(hash, checkpoint.totalBirths);
    HashValue(hash, checkpoint.totalDeaths);

    for (const ColonyView &colony : state->runtime.colonies.GetColonies()) {
        checkpoint.colonies.push_back({
            colony.GetId(),
            colony.GetAntCount(),
            colony.GetReserve(),
            colony.GetFoodQuantity(),
            colony.GetCollectionRate(),
        });

        HashValue(hash, colony.State().id);
        HashValue(hash, colony.Transform().position.x);
        HashValue(hash, colony.Transform().position.y);
        HashValue(hash, colony.State().radius);
        HashValue(hash, colony.State().reserve);
        HashValue(hash, colony.State().foodQuantity);
        HashValue(hash, colony.State().collectionRate);
        HashValue(hash, colony.GetMemberCount());
    }

    for (const AntView &ant : state->runtime.ants.GetAnts()) {
        HashValue(hash, ant.GetId());
        HashValue(hash, ant.Identity().colonyId);
        HashValue(hash, ant.Identity().role);
        HashValue(hash, ant.GetForagingComponent().state);
        HashValue(hash, ant.Transform().position.x);
        HashValue(hash, ant.Transform().position.y);
        HashValue(hash, ant.GetForagingComponent().target.x);
        HashValue(hash, ant.GetForagingComponent().target.y);
        HashValue(hash, ant.GetForagingComponent().walkTime);
        HashValue(hash, ant.GetEnergy());
        HashValue(hash, ant.Motion().speed);
        HashValue(hash, ant.Motion().travelDistance);
        HashValue(hash, ant.GetForagingComponent().collectedFood);
        HashValue(hash, ant.GetEncounterComponent().enemyTimer);
        HashValue(hash, ant.GetForagingComponent().blocked);
        HashValue(hash, ant.GetForagingComponent().timeSinceLastMarker);
        HashValue(hash, ant.GetForagingComponent().distanceToTarget);
        HashValue(hash, ant.GetForagingComponent().lastMarkerPosition.x);
        HashValue(hash, ant.GetForagingComponent().lastMarkerPosition.y);
        HashValue(hash, ant.Pose().direction.GetAngle());
        HashValue(hash, ant.Pose().direction.GetSpeed());
        HashValue(hash, ant.Pose().headDirection.GetAngle());
        HashValue(hash, ant.Pose().tailDirection.GetAngle());
        const bool hasOpponent = ant.GetEncounterComponent().opponentId.has_value();
        HashValue(hash, hasOpponent);
        if (hasOpponent) {
            HashValue(hash, *ant.GetEncounterComponent().opponentId);
        }
    }

    for (const AntPhysicsBody &body : state->physics.bodies.GetBodies()) {
        HashValue(hash, body.id);
        HashValue(hash, body.antId);
        HashValue(hash, body.colonyId);
        HashValue(hash, body.position.x);
        HashValue(hash, body.position.y);
        HashValue(hash, body.previousPosition.x);
        HashValue(hash, body.previousPosition.y);
        HashValue(hash, body.velocity.x);
        HashValue(hash, body.velocity.y);
        HashValue(hash, body.direction.x);
        HashValue(hash, body.direction.y);
        HashValue(hash, body.mass);
    }

    std::map<ColonyId, AntMarkerCheckpoint> markerTotals;
    for (const AntWorldCell &cell : state->runtime.environment.GetCells()) {
        HashValue(hash, cell.wall);
        HashValue(hash, cell.foodQuantity);
        HashValue(hash, cell.markerSamplingCoefficient);

        for (const Marker &marker : cell.markers) {
            HashValue(hash, marker.colonyId);
            HashValue(hash, marker.intensity);
            HashValue(hash, marker.persistent);

            if (!marker.HasOwner()) {
                continue;
            }

            AntMarkerCheckpoint &total = markerTotals[marker.colonyId];
            total.colonyId = marker.colonyId;
            ++total.ownedCellCount;
            total.totalIntensity += marker.intensity;
        }
    }

    for (const auto &[id, marker] : markerTotals) {
        (void)id;
        checkpoint.markers.push_back(marker);
    }

    checkpoint.stateSignature = hash;
    return checkpoint;
}

void AntWorld::RefreshStatistics() {
    if (state == nullptr) {
        statistics = {};
        return;
    }

    statistics.colonyCount =
        state->runtime.colonies
            .GetColonyCount();

    statistics.antCount =
        state->runtime.ants.GetCount();

    statistics.physicsBodyCount =
        state->physics.bodies
            .GetBodyCount();

    statistics.foodEntityCount =
        state->runtime.environment
            .GetFoodEntities()
            .size();

    statistics.totalFoodQuantity =
        state->runtime.environment
            .GetTotalFoodQuantity();

    statistics.wallCount =
        state->runtime.environment
            .GetWallCount();
}

} // namespace ant_simulation
