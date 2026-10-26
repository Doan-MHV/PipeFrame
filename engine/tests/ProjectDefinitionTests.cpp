#include <PipeFrame/Project/Component.h>
#include <PipeFrame/Project/ProjectDefinition.h>
#include <PipeFrame/Project/SceneProjectRuntime.h>
#include <PipeFrame/World/World.h>
#include <cstdlib>
#include <iostream>
using namespace pipeframe;
namespace {
void Check(bool value, const char* message) { if (!value) { std::cerr << message << '\n'; std::exit(1); } }
struct Value : Component<Value> {
    double amount{};
    static auto BuildSchema() { return Define("test.value", "Value")
        .Editable({.key="amount", .displayName="Amount", .kind=PropertyKind::Number, .defaultValue=0.0}, &Value::amount); }
};
struct Recipe : EntityArchetype {
    void Build(ecs::World& world, ecs::Entity entity) const override { world.Add<Value>(entity); }
};
struct TestWorld : World {
    explicit TestWorld(int tag) : World(2), tag(tag) {}
    int tag; int prepared{}; int completed{}; int initialized{};
    BehaviourScene scene;
    void Prepare(float) { ++prepared; }
    void Finish(float) { Check(prepared == completed + 1, "System order changed"); ++completed; }
    void Dispatch(const ProjectDefinition<TestWorld>& project, std::size_t phase, float delta) {
        RunPhase(phase, delta, [&] { project.ExecutePhase(phase, *this, delta); });
    }
};
struct Tick : Behaviour {
    static inline int ticks{};
    void FixedUpdate(float) override { ++ticks; }
};
struct Host : SceneProjectRuntime {
    struct HostWorld {} world;
    Host() : SceneProjectRuntime("Definition test") {
        ProjectDefinition<HostWorld> project;
        project.Components<Value>().Entity<Recipe>({"test.entity", "Entity"}).Behaviour<Tick>("test.tick", "Tick");
        RegisterProject(project, world);
    }
};
}
int main() {
    ProjectDefinition<TestWorld> project;
    project.Components<Value>().Entity<Recipe>({"test.entity", "Entity"})
        .EntityFactory("test.entity", [](TestWorld& world, BehaviourScene&, const SceneObjectData&) {
            auto object = world.scene.Instantiate(Recipe{});
            object.GetComponent<Value>()->amount = world.tag;
            return object;
        }, [](TestWorld&, SceneObject, const SceneObjectData&) {})
        .ConfigureWorld([](TestWorld& world) { ++world.initialized; })
        .Phase({.id="prepare", .phase=SystemPhase::FixedPrePhysics}, &TestWorld::Prepare)
        .Phase({.id="finish", .phase=SystemPhase::FixedCleanup, .dependencies={"prepare"}}, &TestWorld::Finish);
    ComponentRegistry components; EntityRegistry entities;
    project.Install(components, entities);
    Check(components.Contains("test.value") && entities.Contains("test.entity"), "Unified registration missing");
    bool duplicate=false;
    try { project.Install(components, entities); } catch (const std::invalid_argument&) { duplicate=true; }
    Check(duplicate && components.Describe().size()==1 && entities.Describe().size()==1,
          "Duplicate installation must fail without partial registration");
    duplicate=false;
    try { project.Phase({.id="prepare"}, &TestWorld::Prepare); } catch (const std::invalid_argument&) { duplicate=true; }
    Check(duplicate && project.Phases().size()==2, "Duplicate phase accepted");
    // Same immutable definition, different world lifetimes and fresh factory bindings.
    for (int tag : {7, 13}) {
        auto world = std::make_unique<TestWorld>(tag);
        project.InitializeWorld(*world);
        auto bound = entities;
        project.BindFactories(bound, *world);
        SceneObjectData data; data.id=1; data.typeId="test.entity";
        const auto object = bound.Instantiate(world->scene, data, components);
        Check(object.GetComponent<Value>()->amount==tag, "Factory retained the old world");
        bool rejected=false;
        try { world->Dispatch(project, 1, .01f); } catch (const std::logic_error&) { rejected=true; }
        Check(rejected, "Skipped phase accepted");
        for(int tick=0; tick<3; ++tick) {
            for(std::size_t phase=0; phase<project.Phases().size(); ++phase) world->Dispatch(project,phase,.01f);
        }
        Check(world->initialized==1 && world->prepared==3 && world->completed==3, "System ran twice or reset retained schedule state");
    }
    // Definition-registered behaviours attach exactly once through the shared runtime,
    // and survive scene reset/recreation without depending on generated legacy maps.
    Host host;
    SceneObjectData object; object.id=1; object.typeId="test.entity";
    object.components.push_back({"test.tick", true, {}});
    host.SynchronizeScene(std::span(&object,1)); host.Start(); host.FixedUpdate(.01f);
    Check(Tick::ticks==1, "Definition behaviour must tick once");
    host.Reset(); host.Start(); host.FixedUpdate(.01f);
    Check(Tick::ticks==2, "Definition behaviour must survive reset exactly once");
    std::cout << "Unified project schemas/entities, transactional registration, phases, reset and factory lifetimes passed\n";
}
