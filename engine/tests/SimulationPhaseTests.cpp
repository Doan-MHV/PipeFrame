#include <PipeFrame/Project/SceneProjectRuntime.h>
#include <PipeFrame/World/World.h>

#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
using namespace pipeframe;

void Check(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

struct Probe final : FixedUpdateSystem<void> {
  explicit Probe(std::function<void(float)> update) : update(std::move(update)) {}
  std::string_view GetSystemId() const override { return "test.phase"; }
  void Update(float delta) override { update(delta); }
  std::function<void(float)> update;
};

template <class Result>
struct ResultProbe final : FixedUpdateSystem<Result> {
  explicit ResultProbe(std::function<Result(float)> update) : update(std::move(update)) {}
  std::string_view GetSystemId() const override { return "test.result-phase"; }
  Result Update(float delta) override { return update(delta); }
  std::function<Result(float)> update;
};

struct Script final : Behaviour {
  explicit Script(std::vector<int>& log) : log(log) {}
  void Start() override { log.push_back(1); }
  void FixedUpdate(float) override { log.push_back(2); }
  std::vector<int>& log;
};

class Host final : public SceneProjectRuntime {
public:
  Host() : SceneProjectRuntime("Phase test"), simulation(GetRuntimeScene()) {
    SetSimulationWorld(simulation);
    // Intentionally interleave registrations: phase order wins over global order.
    simulation.AddSystem(afterA, UpdatePhase::AfterPhysics);
    simulation.AddSystem(cleanup, UpdatePhase::Cleanup, [this](int value) {
      Check(value == 8, "Cleanup result was not delivered");
      cleanupResult = value;
      log.push_back(9);
    });
    simulation.AddSystem(before);
    simulation.AddSystem(early, UpdatePhase::BeforeScene);
    simulation.AddSystem(afterB, UpdatePhase::AfterPhysics);
    simulation.AddSystem(cleanupAfter, UpdatePhase::Cleanup);
    GetRuntimeScene().CreateObject().Attach<Script>(log);
  }

  void BindWrongScene() {
    SimulationWorld wrong;
    SetSimulationWorld(wrong);
  }

  std::vector<int> log;
  bool failPhysics{};
  bool failAfterPhysics{};
  int physicsResult{};
  int cleanupResult{};
  Probe early{[this](float) { log.push_back(0); }};
  Probe before{[this](float dt) {
    Check(dt == .02f, "Fixed delta changed before physics");
    physicsResult = 1;
    log.push_back(3);
  }};
  Probe afterA{[this](float dt) {
    Check(dt == .02f && physicsResult == 3, "Post systems ran before complete physics");
    physicsResult = 4;
    log.push_back(6);
    if (failAfterPhysics) throw std::runtime_error("test post-physics failure");
  }};
  Probe afterB{[this](float) {
    Check(physicsResult == 4, "Post-phase registration order changed");
    log.push_back(7);
  }};
  ResultProbe<int> cleanup{[this](float dt) {
    Check(dt == .02f && log.back() == 7, "Cleanup ran before post-physics systems");
    log.push_back(8);
    return 8;
  }};
  Probe cleanupAfter{[this](float) {
    Check(cleanupResult == 8 && log.back() == 9, "Cleanup result arrived after the next system");
    log.push_back(10);
  }};
  SimulationWorld simulation;

protected:
  void UpdatePhysics(float delta, const PreviousTransforms& previous) override {
    Check(physicsResult == 1, "Controller did not run before physics");
    log.push_back(4);
    if (failPhysics) throw std::runtime_error("test physics failure");
    SceneProjectRuntime::UpdatePhysics(delta, previous);
    physicsResult = 2;
    // A custom solver/extra project physics step must finish before post systems.
    log.push_back(5);
    physicsResult = 3;
  }
};

void CheckHost() {
  Host host;
  bool rejected = false;
  try { host.BindWrongScene(); } catch (const std::invalid_argument&) { rejected = true; }
  Check(rejected, "Foreign scene accepted by the runtime");
  host.FixedUpdate(.02f);
  Check(host.log.empty(), "Stopped runtime dispatched a phase");
  host.Start();
  for (float dt : {0.f, -.02f, std::numeric_limits<float>::quiet_NaN(),
                   std::numeric_limits<float>::infinity()}) host.FixedUpdate(dt);
  Check(host.log.empty(), "Invalid delta dispatched a phase");
  host.FixedUpdate(.02f);
  Check(host.log == std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, "First tick phase order");
  host.log.clear();
  host.FixedUpdate(.02f);
  Check(host.log == std::vector<int>{0, 2, 3, 4, 5, 6, 7, 8, 9, 10}, "Repeated or missing dispatch");
  host.Stop();
  host.log.clear();
  host.FixedUpdate(.02f);
  Check(host.log.empty(), "Pause dispatched systems");
  host.Start();
  host.FixedUpdate(.02f);
  Check(host.log == std::vector<int>{0, 2, 3, 4, 5, 6, 7, 8, 9, 10}, "Resume duplicated scheduling");

  // The editor stops before resetting. Reset itself preserves playback state.
  host.Stop();
  host.Reset(); // The direct test behaviour is not authored, so reset removes it.
  host.log.clear();
  host.FixedUpdate(.02f);
  Check(host.log.empty(), "Reset should preserve the stopped runtime state");
  host.Start();
  host.FixedUpdate(.02f);
  Check(host.log == std::vector<int>{0, 3, 4, 5, 6, 7, 8, 9, 10}, "Reset lost or duplicated registrations");
  host.Unload();
  host.log.clear();
  host.Start();
  host.FixedUpdate(.02f);
  Check(host.log == std::vector<int>{0, 3, 4, 5, 6, 7, 8, 9, 10}, "Unload lost scheduling connection");

  host.log.clear();
  host.failPhysics = true;
  rejected = false;
  try { host.FixedUpdate(.02f); } catch (const std::runtime_error&) { rejected = true; }
  Check(rejected && host.log == std::vector<int>{0, 3, 4}, "Post phase ran after failed physics");
  host.failPhysics = false;
  host.log.clear();
  host.FixedUpdate(.02f);
  Check(host.log == std::vector<int>{0, 3, 4, 5, 6, 7, 8, 9, 10}, "Dispatch did not recover after exception");

  host.log.clear();
  host.failAfterPhysics = true;
  rejected = false;
  try { host.FixedUpdate(.02f); } catch (const std::runtime_error&) { rejected = true; }
  Check(rejected && host.log == std::vector<int>{0, 3, 4, 5, 6},
    "Cleanup ran after a failed post-physics system");
  host.failAfterPhysics = false;
  host.log.clear();
  host.FixedUpdate(.02f);
  Check(host.log == std::vector<int>{0, 3, 4, 5, 6, 7, 8, 9, 10},
    "Post-physics exception kept cleanup from resuming");
}

void CheckStandalone() {
  SimulationWorld world;
  std::vector<int> log;
  Probe pre{[&](float) { log.push_back(1); }};
  Probe regular{[&](float) { log.push_back(2); }};
  Probe post{[&](float) { log.push_back(3); }};
  Probe cleanup{[&](float) { log.push_back(4); }};
  world.AddPreSceneSystem(pre); // Existing API remains compatible.
  world.AddSystem(regular);
  world.AddSystem(post, UpdatePhase::AfterPhysics);
  world.AddSystem(cleanup, UpdatePhase::Cleanup);
  bool rejected = false;
  try { world.AddSystem(regular, UpdatePhase::AfterPhysics); }
  catch (const std::invalid_argument&) { rejected = true; }
  Check(rejected, "Same system registered twice across phases");
  Probe unused{[](float) {}};
  rejected = false;
  try { world.AddSystem(unused, static_cast<UpdatePhase>(99)); }
  catch (const std::invalid_argument&) { rejected = true; }
  Check(rejected, "Invalid phase accepted");
  for (float dt : {0.f, -.02f, std::numeric_limits<float>::quiet_NaN(),
                   std::numeric_limits<float>::infinity()}) {
    world.Update(dt);
    world.UpdateAfterPhysics(dt);
    world.UpdateCleanup(dt);
  }
  Check(log.empty(), "Standalone world accepted invalid delta");
  world.Update(.02f);
  Check(log == std::vector<int>{1, 2}, "Legacy Update ran post-physics systems");
  world.UpdateAfterPhysics(.02f);
  Check(log == std::vector<int>{1, 2, 3}, "Standalone post phase reran scene or pre phase");
  world.UpdateCleanup(.02f);
  Check(log == std::vector<int>{1, 2, 3, 4}, "Standalone cleanup reran another phase");

  SimulationWorld guarded;
  bool mutate = true;
  Probe mutation{[&](float) { if (mutate) guarded.AddSystem(unused); }};
  guarded.AddSystem(mutation);
  rejected = false;
  try { guarded.Update(.02f); } catch (const std::logic_error&) { rejected = true; }
  Check(rejected, "Registration mutated the active dispatch list");
  mutate = false;
  guarded.Update(.02f); // Exception must release the dispatch guard.
  guarded.AddSystem(unused);

  Probe recursive{[&](float dt) { guarded.Update(dt); }};
  guarded.AddSystem(recursive, UpdatePhase::AfterPhysics);
  rejected = false;
  try { guarded.UpdateAfterPhysics(.02f); } catch (const std::logic_error&) { rejected = true; }
  Check(rejected, "Reentrant dispatch accepted");

  Probe recursiveCleanup{[&](float dt) { guarded.UpdateAfterPhysics(dt); }};
  guarded.AddSystem(recursiveCleanup, UpdatePhase::Cleanup);
  rejected = false;
  try { guarded.UpdateCleanup(.02f); } catch (const std::logic_error&) { rejected = true; }
  Check(rejected, "Cleanup allowed reentrant dispatch of another phase");
}

void CheckResults() {
  SimulationWorld world;
  int runs = 0;
  int received = 0;
  bool failSink = true;
  ResultProbe<std::unique_ptr<int>> result{[&](float dt) {
    Check(dt == .02f, "Result system received the wrong fixed delta");
    return std::make_unique<int>(++runs);
  }};
  Probe unused{[](float) {}};
  world.AddSystem(result, UpdatePhase::Cleanup, [&](std::unique_ptr<int> value) {
    received = *value;
    if (failSink) throw std::runtime_error("test sink failure");
    bool rejected = false;
    try { world.AddSystem(unused); } catch (const std::logic_error&) { rejected = true; }
    Check(rejected, "Result sink mutated the active registration list");
  });
  bool rejected = false;
  try { world.AddSystem(result); } catch (const std::invalid_argument&) { rejected = true; }
  Check(rejected, "Result system registered twice across phases/overloads");
  ResultProbe<int> discarded{[&](float) { ++runs; return 42; }};
  rejected = false;
  try { world.AddSystem(discarded, static_cast<UpdatePhase>(99), [](int) {}); }
  catch (const std::invalid_argument&) { rejected = true; }
  Check(rejected, "Result-system registration accepted an invalid phase");
  world.AddSystem(discarded); // Non-void systems can omit the result sink.
  for (float dt : {0.f, -.02f, std::numeric_limits<float>::quiet_NaN(),
                   std::numeric_limits<float>::infinity()}) world.UpdateCleanup(dt);
  Check(runs == 0 && received == 0, "Invalid cleanup delta invoked a system or sink");
  rejected = false;
  try { world.UpdateCleanup(.02f); } catch (const std::runtime_error&) { rejected = true; }
  Check(rejected && runs == 1 && received == 1, "Move-only result or sink exception was lost");
  failSink = false;
  world.UpdateCleanup(.02f);
  Check(runs == 2 && received == 2, "Sink exception did not release dispatch guard");
  world.Update(.02f);
  Check(runs == 3 && received == 2, "Discarded-result system did not run independently");
}

void CheckPhysics() {
  PhysicsWorld world;
  std::vector<int> log;
  int received = 0;
  Probe first{[&](float) { log.push_back(1); }};
  ResultProbe<int> result{[&](float dt) {
    Check(dt == .02f, "Physics result system received the wrong fixed delta");
    log.push_back(2);
    return 42;
  }};
  ResultProbe<int> discarded{[&](float) { log.push_back(5); return 0; }};
  world.AddSystem(first);
  world.AddSystem(result, [&](int value) { received = value; log.push_back(3); });
  world.AddStep([&](float) {
    Check(received == 42, "Physics result was not delivered before the next step");
    log.push_back(4);
  });
  world.AddSystem(discarded);
  bool rejected = false;
  try { world.AddSystem(result); } catch (const std::invalid_argument&) { rejected = true; }
  Check(rejected, "Physics result system registered twice");
  rejected = false;
  try { world.AddSystem(first); } catch (const std::invalid_argument&) { rejected = true; }
  Check(rejected, "Void physics system registered twice");
  rejected = false;
  try { world.AddStep({}); } catch (const std::invalid_argument&) { rejected = true; }
  Check(rejected, "Empty physics step accepted");
  world.Update(0.f);
  world.Update(-.02f);
  for (float dt : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
    rejected = false;
    try { world.Update(dt); } catch (const std::invalid_argument&) { rejected = true; }
    Check(rejected, "Physics changed its non-finite timestep rejection contract");
  }
  Check(log.empty(), "Invalid physics delta dispatched a system or sink");
  world.Update(.02f);
  Check(log == std::vector<int>{1, 2, 3, 4, 5}, "Physics AddSystem/AddStep ordering changed");

  PhysicsWorld guarded;
  ResultProbe<int> mutation{[](float) { return 1; }};
  Probe unused{[](float) {}};
  bool systemMutationRejected = false;
  bool stepMutationRejected = false;
  guarded.AddSystem(mutation, [&](int) {
    try { guarded.AddSystem(unused); } catch (const std::logic_error&) { systemMutationRejected = true; }
    try { guarded.AddStep([](float) {}); } catch (const std::logic_error&) { stepMutationRejected = true; }
  });
  guarded.Update(.02f);
  Check(systemMutationRejected && stepMutationRejected,
    "Physics result sink mutated the active registration list");
  guarded.AddSystem(unused); // Completed dispatch releases the registration guard.

  PhysicsWorld recursive;
  Probe recursion{[&](float dt) { recursive.Update(dt); }};
  recursive.AddSystem(recursion);
  rejected = false;
  try { recursive.Update(.02f); } catch (const std::logic_error&) { rejected = true; }
  Check(rejected, "Physics system reentered dispatch");
  rejected = false;
  try { recursive.Update(.02f); } catch (const std::logic_error&) { rejected = true; }
  Check(rejected, "Failed physics step lost its faulted-state protection");
}
} // namespace

int main() {
  try {
    CheckStandalone();
    CheckHost();
    CheckResults();
    CheckPhysics();
    std::cout << "Simulation phases: cleanup, results, lifecycle, compatibility and guards passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
