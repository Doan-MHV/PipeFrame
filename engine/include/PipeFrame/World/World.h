#pragma once
#include <PipeFrame/Core/FixedStepSequence.h>
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Render/Canvas.h>
#include <PipeFrame/Render/RenderSystem.h>
#include <PipeFrame/Simulation/System.h>

#include <functional>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
namespace pipeframe {
class RenderingWorld;
// One owner controls phase order. Child modules contain the smaller systems.
class World {
public:
    virtual ~World() = default;
    void SetRenderingWorld(std::shared_ptr<RenderingWorld> value) { rendering = std::move(value); }
    RenderingWorld* GetRenderingWorld() const { return rendering.get(); }

protected:
    explicit World(std::size_t phases = 4) : phases(phases), sequence(phases) {}
    void ResetSchedule() { sequence = FixedStepSequence(phases); }
    template <class F>
    void RunPhase(std::size_t phase, float delta, F run) {
        sequence.Execute(phase, delta, run);
    }

private:
    std::shared_ptr<RenderingWorld> rendering;
    std::size_t phases;
    FixedStepSequence sequence;
};

class SimulationWorld {
public:
    SimulationWorld() = default;
    explicit SimulationWorld(BehaviourScene& sharedScene) : activeScene(&sharedScene) {}
    SimulationWorld(const SimulationWorld&) = delete;
    SimulationWorld& operator=(const SimulationWorld&) = delete;
    virtual ~SimulationWorld() = default;
    BehaviourScene& GetScene() { return *activeScene; }
    // Non-owning: systems must outlive their registration. A system belongs to
    // exactly one phase. Existing AddSystem(system) calls remain before physics.
    void AddSystem(FixedUpdateSystem<void>& system, UpdatePhase phase = UpdatePhase::BeforePhysics) {
        RegisterSystem(&system, phase, [&system](float delta) { system.Update(delta); });
    }
    template <class Result> requires (!std::is_void_v<Result>)
    void AddSystem(FixedUpdateSystem<Result>& system, UpdatePhase phase = UpdatePhase::BeforePhysics) {
        RegisterSystem(&system, phase, [&system](float delta) { (void)system.Update(delta); });
    }
    // Results are delivered immediately after this system, before the next one.
    // The sink is stored by value; referenced state must outlive registration.
    template <class Result, class Sink>
        requires (!std::is_void_v<Result> && std::is_invocable_v<Sink&, Result>)
    void AddSystem(FixedUpdateSystem<Result>& system, UpdatePhase phase, Sink sink) {
        RegisterSystem(&system, phase,
            [&system, sink = std::move(sink)](float delta) mutable {
                std::invoke(sink, system.Update(delta));
            });
    }
    // Compatibility for existing projects; new code can use one registration API.
    void AddPreSceneSystem(FixedUpdateSystem<void>& system) { AddSystem(system, UpdatePhase::BeforeScene); }
    void Update(float delta) { Dispatch(delta, UpdatePhase::BeforePhysics); }
    // SceneProjectRuntime calls these after the complete physics override returns.
    // Standalone owners call both once, in this order, after their physics step.
    void UpdateAfterPhysics(float delta) { Dispatch(delta, UpdatePhase::AfterPhysics); }
    void UpdateCleanup(float delta) { Dispatch(delta, UpdatePhase::Cleanup); }

private:
    struct Entry {
        const void* system;
        UpdatePhase phase;
        std::function<void(float)> update;
    };
    void RegisterSystem(const void* system, UpdatePhase phase, std::function<void(float)> update) {
        if (updating) throw std::logic_error("Cannot register systems during simulation dispatch");
        if (phase != UpdatePhase::BeforeScene && phase != UpdatePhase::BeforePhysics &&
            phase != UpdatePhase::AfterPhysics && phase != UpdatePhase::Cleanup)
            throw std::invalid_argument("Unknown simulation update phase");
        for (const auto& entry : systems)
            if (entry.system == system) throw std::invalid_argument("Simulation system already registered");
        systems.push_back({system, phase, std::move(update)});
    }
    void RunSystems(UpdatePhase phase, float delta) {
        for (const auto& entry : systems)
            if (entry.phase == phase) entry.update(delta);
    }
    void Dispatch(float delta, UpdatePhase phase) {
        if (!std::isfinite(delta) || delta <= 0) return;
        if (updating) throw std::logic_error("Simulation dispatch cannot be reentered");
        struct Guard {
            bool& updating;
            ~Guard() { updating = false; }
        } guard{updating};
        updating = true;
        if (phase == UpdatePhase::BeforePhysics) {
            RunSystems(UpdatePhase::BeforeScene, delta);
            activeScene->FixedUpdate(delta);
            RunSystems(UpdatePhase::BeforePhysics, delta);
        } else {
            RunSystems(phase, delta);
        }
    }
    BehaviourScene scene;
    BehaviourScene* activeScene{&scene};
    std::vector<Entry> systems;
    bool updating{};
};

// Compatibility for projects generated before the SimulationWorld naming update.
using RuntimeWorld = SimulationWorld;

class PhysicsWorld {
public:
    PhysicsWorld() = default;
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    virtual ~PhysicsWorld() = default;
    void Update(float delta) {
        if (delta <= 0) return;
        sequence.Execute(0, delta, [&] {
            struct Guard {
                bool& updating;
                ~Guard() { updating = false; }
            } guard{updating};
            updating = true;
            for (auto& step : steps)
                step.update(delta);
        });
    }

public:
    void AddSystem(FixedUpdateSystem<void>& system) {
        RegisterStep(&system, [&system](float delta) { system.Update(delta); });
    }
    template <class Result> requires (!std::is_void_v<Result>)
    void AddSystem(FixedUpdateSystem<Result>& system) {
        RegisterStep(&system, [&system](float delta) { (void)system.Update(delta); });
    }
    template <class Result, class Sink>
        requires (!std::is_void_v<Result> && std::is_invocable_v<Sink&, Result>)
    void AddSystem(FixedUpdateSystem<Result>& system, Sink sink) {
        RegisterStep(&system, [&system, sink = std::move(sink)](float delta) mutable {
            std::invoke(sink, system.Update(delta));
        });
    }
    // Existing custom callbacks remain supported in registration order.
    void AddStep(std::function<void(float)> step) { RegisterStep(nullptr, std::move(step)); }

private:
    struct Entry {
        const void* system;
        std::function<void(float)> update;
    };
    void RegisterStep(const void* system, std::function<void(float)> step) {
        if (updating) throw std::logic_error("Cannot register systems during physics dispatch");
        if (!step) throw std::invalid_argument("Physics step requires a callback");
        if (system)
            for (const auto& entry : steps)
                if (entry.system == system) throw std::invalid_argument("Physics system already registered");
        steps.push_back({system, std::move(step)});
    }
    FixedStepSequence sequence{1};
    std::vector<Entry> steps;
    bool updating{};
};

class RenderLayer {
public:
    virtual ~RenderLayer() = default;
    void SetLayerEnabled(bool value) { enabled = value; }
    void Render(Canvas canvas, RenderState states = {}) const {
        if (enabled) Draw(canvas, states);
    }
    virtual void Draw(Canvas, RenderState = {}) const = 0;

private:
    bool enabled{true};
};

class RenderingWorld {
public:
    RenderingWorld() = default;
    RenderingWorld(const RenderingWorld&) = delete;
    RenderingWorld& operator=(const RenderingWorld&) = delete;
    virtual ~RenderingWorld() = default;
    void Render(Canvas canvas, int stage = 0) const {
        for (const auto& entry : layers)
            if (entry.stage == stage) entry.layer->Render(canvas);
    }

    // Non-owning registrations; the project world owns the systems.
    void RenderSystems(Canvas canvas, BehaviourScene& scene, RenderOptions options = {}) {
        for (auto* system : systems) system->Render(canvas, scene, options);
    }

protected:
    void AddSystem(RenderSystem& system) { systems.push_back(&system); }
    void AddLayer(RenderLayer& layer, int stage = 0) { layers.push_back({&layer, stage}); }

private:
    struct Entry {
        RenderLayer* layer;
        int stage;
    };
    std::vector<Entry> layers;
    std::vector<RenderSystem*> systems;
};
}  // namespace pipeframe
