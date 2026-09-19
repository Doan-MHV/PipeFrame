#pragma once
#include <PipeFrame/Core/FixedStepSequence.h>
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Render/Canvas.h>
#include <PipeFrame/Simulation/System.h>

#include <functional>
#include <memory>
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

class RuntimeWorld {
public:
    RuntimeWorld() = default;
    explicit RuntimeWorld(BehaviourScene& sharedScene) : activeScene(&sharedScene) {}
    RuntimeWorld(const RuntimeWorld&) = delete;
    RuntimeWorld& operator=(const RuntimeWorld&) = delete;
    virtual ~RuntimeWorld() = default;
    BehaviourScene& GetScene() { return *activeScene; }
    void AddSystem(FixedUpdateSystem<void>& system) { systems.push_back(&system); }
    void AddPreSceneSystem(FixedUpdateSystem<void>& system) { preSceneSystems.push_back(&system); }
    void Update(float delta) {
        if (delta <= 0) return;
        for (auto* system : preSceneSystems)
            system->Update(delta);
        activeScene->FixedUpdate(delta);
        for (auto* system : systems)
            system->Update(delta);
    }

private:
    BehaviourScene scene;
    BehaviourScene* activeScene{&scene};
    std::vector<FixedUpdateSystem<void>*> preSceneSystems, systems;
};

class PhysicsWorld {
public:
    PhysicsWorld() = default;
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    virtual ~PhysicsWorld() = default;
    void Update(float delta) {
        if (delta <= 0) return;
        sequence.Execute(0, delta, [&] {
            for (auto& step : steps)
                step(delta);
        });
    }

protected:
    void AddStep(std::function<void(float)> step) { steps.push_back(std::move(step)); }

private:
    FixedStepSequence sequence{1};
    std::vector<std::function<void(float)>> steps;
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

protected:
    void AddLayer(RenderLayer& layer, int stage = 0) { layers.push_back({&layer, stage}); }

private:
    struct Entry {
        RenderLayer* layer;
        int stage;
    };
    std::vector<Entry> layers;
};
}  // namespace pipeframe
