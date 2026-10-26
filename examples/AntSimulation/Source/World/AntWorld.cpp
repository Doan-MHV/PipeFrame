#include "World/AntWorld.h"

#include <PipeFrame/Core/FixedStepSequence.h>

#include <bit>
#include <cassert>
#include <chrono>
#include <map>
#include <type_traits>
#include <utility>

#include "Integration/AntProject.h"
#include "World/Physics/AntPhysicsWorld.h"
#include "World/Physics/ContactSolver.h"
#include "World/Simulation/AntSimulationWorld.h"

namespace ant_simulation {

namespace {

void HashBytes(std::uint64_t& hash, const void* data, const std::size_t size) {
  const auto* bytes = static_cast<const unsigned char*>(data);
  for (std::size_t index = 0; index < size; ++index) {
    hash ^= bytes[index];
    hash *= 1099511628211ULL;
  }
}

template <typename Value>
void HashValue(std::uint64_t& hash, const Value& value) {
  static_assert(std::is_trivially_copyable_v<Value>);
  HashBytes(hash, &value, sizeof(Value));
}

}  // namespace

struct AntWorld::State {
  State(const AntConfiguration& configuration, std::uint32_t seed)
    : simulation(configuration, seed),
      physics(simulation.ants, simulation.environment, configuration) {
    simulation.BindPhysics(physics.bodies, configuration);
  }
  AntSimulationWorld simulation;
  AntPhysicsWorld physics;
};

AntWorld::AntWorld(AntConfiguration newConfiguration, const std::uint32_t newRandomSeed)
  : pipeframe::World(AntProject::Definition().Phases().size()),
    configuration(std::move(newConfiguration)),
    randomSeed(newRandomSeed) {}

AntPhysicsWorld& AntWorld::GetPhysicsWorld() {
  assert(state);
  return state->physics;
}
AntSimulationWorld& AntWorld::GetSimulationWorld() {
  assert(state);
  return state->simulation;
}

AntWorld::~AntWorld() = default;

bool AntWorld::Initialize(std::string& errorMessage) {
  std::unique_ptr<State> newState = std::make_unique<State>(configuration, randomSeed);

  if (!newState->simulation.environment.Initialize(configuration, errorMessage)) {
    state.reset();
    statistics = {};

    return false;
  }

  state = std::move(newState);
  AntProject::Definition().InitializeWorld(*this);
  ResetSchedule();
  statistics = {};

  RefreshStatistics();

  errorMessage.clear();

  return true;
}

bool AntWorld::Reset(std::string& errorMessage) {
  return Initialize(errorMessage);
}

void AntWorld::Clear() {
  state.reset();
  statistics = {};
}

ColonyView& AntWorld::CreateColony(
  const ColonyId id, const pipeframe::Vector2f position, const pipeframe::Color color) {
  assert(state != nullptr);

  ColonyView& colony = state->simulation.colonies.CreateColony(id, position, color);

  RefreshStatistics();

  return colony;
}

bool AntWorld::RemoveColony(const ColonyId id) {
  if (state == nullptr || state->simulation.colonies.FindColony(id) == nullptr) {
    return false;
  }

  const std::size_t removedAnts = state->simulation.colonies.RemoveColony(id);
  statistics.totalDeaths += removedAnts;
  state->physics.bodies.Synchronize();
  RefreshStatistics();

  return true;
}

ColonyView& AntWorld::CreateColony(
  const pipeframe::Vector2f position, const pipeframe::Color color) {
  assert(state != nullptr);

  ColonyView& colony = state->simulation.colonies.CreateColony(position, color);

  RefreshStatistics();

  return colony;
}

AntSimulationStepResult AntWorld::FixedUpdate(const float fixedDeltaTime) {
  if (state == nullptr || fixedDeltaTime <= 0.0f) {
    return statistics.lastAntUpdate;
  }

  for (std::size_t phase = 0; phase < AntProject::Definition().Phases().size(); ++phase)
    ExecuteProjectPhase(phase, fixedDeltaTime);

  return statistics.lastAntUpdate;
}

void AntWorld::ExecuteProjectPhase(std::size_t phase, float delta) {
  if (!state || delta <= 0.0f) return;
  RunPhase(phase, delta, [&] { AntProject::Definition().ExecutePhase(phase, *this, delta); });
}
// Compatibility with callers that step the four phases individually. All paths
// dispatch the same immutable definition; these wrappers contain no system logic.
void AntWorld::BeginFixedStep(float delta) {
  ExecuteProjectPhase(0, delta);
}
void AntWorld::UpdateMovement(float delta) {
  ExecuteProjectPhase(1, delta);
}
void AntWorld::UpdateBehavior(float delta) {
  ExecuteProjectPhase(2, delta);
}
void AntWorld::EndFixedStep(float delta) {
  ExecuteProjectPhase(3, delta);
}

void AntWorld::Prepare(const float fixedDeltaTime) {
  const std::size_t before = state->simulation.ants.GetCount();
  state->simulation.Update(fixedDeltaTime);
  statistics.totalBirths += state->simulation.ants.GetCount() - before;
}

void AntWorld::Move(const float fixedDeltaTime) {
  state->physics.Update(fixedDeltaTime);
  const AntMovementResult movement = state->physics.lastResult;
  auto& result = statistics.lastAntUpdate;
  result.antContacts = movement.antContacts;
  result.wallConstraints = movement.wallConstraints;
  result.futureCollisions = movement.futureCollisions;
  result.preparationTimeMs = movement.preparationTimeMs;
  result.avoidanceTimeMs = movement.avoidanceTimeMs;
  result.physicsTimeMs = movement.physicsTimeMs;
}

void AntWorld::Forage(const float fixedDeltaTime) {
  const auto started = std::chrono::steady_clock::now();
  state->simulation.UpdateAfterPhysics(fixedDeltaTime);
  statistics.lastAntUpdate.behaviorTimeMs =
    std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - started).count();
}

void AntWorld::Cleanup(const float fixedDeltaTime) {
  const auto started = std::chrono::steady_clock::now();
  state->simulation.UpdateCleanup(fixedDeltaTime);
  const AntCleanupResult cleanup = state->simulation.lastCleanupResult;
  auto& result = statistics.lastAntUpdate;
  result.cleanupTimeMs =
    std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - started).count();
  result.enemyAlerts = cleanup.enemyAlerts;
  result.removedAnts = cleanup.removedAnts;
  result.livingAnts = state->simulation.ants.GetCount();
  statistics.totalDeaths += cleanup.removedAnts;
  ++statistics.tick;
  RefreshStatistics();
}

bool AntWorld::IsInitialized() const {
  return state != nullptr && state->simulation.environment.IsInitialized();
}

const AntConfiguration& AntWorld::GetConfiguration() const {
  return configuration;
}

AntEnvironment& AntWorld::GetEnvironment() {
  assert(state != nullptr);
  return state->simulation.environment;
}

const AntEnvironment& AntWorld::GetEnvironment() const {
  assert(state != nullptr);
  return state->simulation.environment;
}

pipeframe::BehaviourScene& AntWorld::GetScene() {
  assert(state != nullptr);
  return state->simulation.GetScene();
}

AntQuery& AntWorld::GetAntQuery() {
  assert(state != nullptr);
  return state->simulation.ants;
}

const AntQuery& AntWorld::GetAntQuery() const {
  assert(state != nullptr);
  return state->simulation.ants;
}

ColonyLifecycleSystem& AntWorld::GetColonyLifecycleSystem() {
  assert(state != nullptr);
  return state->simulation.colonies;
}

const ColonyLifecycleSystem& AntWorld::GetColonyLifecycleSystem() const {
  assert(state != nullptr);
  return state->simulation.colonies;
}

AntBodySystem& AntWorld::GetPhysicsBodies() {
  assert(state != nullptr);
  return state->physics.bodies;
}

const AntBodySystem& AntWorld::GetPhysicsBodies() const {
  assert(state != nullptr);
  return state->physics.bodies;
}

const AntSimulationWorldStatistics& AntWorld::GetStatistics() const {
  return statistics;
}

AntBehaviorCheckpoint AntWorld::CaptureBehaviorCheckpoint() const {
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

  for (const ColonyView& colony : state->simulation.colonies.GetColonies()) {
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

  for (const AntView& ant : state->simulation.ants.GetAnts()) {
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

  for (const AntPhysicsBody& body : state->physics.bodies.GetBodies()) {
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
  for (const AntWorldCell& cell : state->simulation.environment.GetCells()) {
    HashValue(hash, cell.wall);
    HashValue(hash, cell.foodQuantity);
    HashValue(hash, cell.markerSamplingCoefficient);

    for (const Marker& marker : cell.markers) {
      HashValue(hash, marker.colonyId);
      HashValue(hash, marker.intensity);
      HashValue(hash, marker.persistent);

      if (!marker.HasOwner()) {
        continue;
      }

      AntMarkerCheckpoint& total = markerTotals[marker.colonyId];
      total.colonyId = marker.colonyId;
      ++total.ownedCellCount;
      total.totalIntensity += marker.intensity;
    }
  }

  for (const auto& [id, marker] : markerTotals) {
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

  statistics.colonyCount = state->simulation.colonies.GetColonyCount();

  statistics.antCount = state->simulation.ants.GetCount();

  statistics.physicsBodyCount = state->physics.bodies.GetBodyCount();

  statistics.foodEntityCount = state->simulation.environment.GetFoodEntities().size();

  statistics.totalFoodQuantity = state->simulation.environment.GetTotalFoodQuantity();

  statistics.wallCount = state->simulation.environment.GetWallCount();
}

}  // namespace ant_simulation
