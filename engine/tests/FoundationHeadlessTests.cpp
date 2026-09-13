#include <PipeFrame/ECS/ComponentView.h>
#include <PipeFrame/ECS/SceneViewCache.h>
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Simulation/Steering2D.h>
#include <PipeFrame/Simulation/DirectionTracker.h>
#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/Input/InputEvent.h>
#include <PipeFrame/Project/ProjectTypes.h>
#include <PipeFrame/Render/RenderTypes.h>
#include <PipeFrame/UI/ViewModels.h>

#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Core/FixedStepSequence.h>
#include <PipeFrame/UI/View.h>
#include <PipeFrame/UI/StatefulView.h>
#include <PipeFrame/Entities/EntityArchetype.h>
#include <stdexcept>
// Lifecycle and structural mutations must execute in Release too.
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error(#__VA_ARGS__); } while (false)
#include <type_traits>

int main() {
    using namespace pipeframe;
    struct Health { int value{10}; };
    BehaviourScene storage;
    const auto storedFirst = storage.CreateObject();
    const auto storedSecond = storage.CreateObject();
    storedFirst.AddComponent<Transform2DComponent>().position = {1, 2};
    storedSecond.AddComponent<Transform2DComponent>().position = {3, 4};
    storedSecond.AddComponent<Health>();
    storage.Destroy(storedFirst.GetEntity());
    CHECK(!storedFirst.IsValid() && storedSecond.IsValid());
    storage.Components().Each<Transform2DComponent, Health>([](auto, auto &pose, auto &health) {
        pose.position = {8, 9}; health.value -= 2;
    });
    CHECK(storedSecond.GetComponent<Health>()->value == 8);
    storage.Destroy(storedSecond.GetEntity());
    CHECK(!storedSecond.IsValid());

    struct Lifecycle final : Behaviour {
        explicit Lifecycle(std::vector<int> &events) : events(events) {}
        void OnEnable() override { events.push_back(1); }
        void Start() override { events.push_back(2); }
        void FixedUpdate(float) override { ++GetComponent<Health>()->value; events.push_back(3); }
        void OnDisable() override { events.push_back(4); }
        void OnDestroy() override { events.push_back(5); }
        std::vector<int> &events;
    };
    std::vector<int> events;
    BehaviourScene scene;
    const auto object = scene.Create();
    scene.Components().Add<Health>(object);
    auto &script = scene.Attach<Lifecycle>(object, events);
    scene.FixedUpdate(0.1f);
    scene.FixedUpdate(0.1f);
    scene.SetEnabled(script, false); scene.FixedUpdate(0.1f);
    scene.SetEnabled(script, true); scene.FixedUpdate(0.1f);
    CHECK(scene.Components().Get<Health>(object)->value == 13);
    scene.Destroy(object);
    CHECK((events == std::vector<int>{1, 2, 3, 3, 4, 1, 3, 4, 5}));
    CHECK(!scene.Components().IsAlive(object));
    struct SelfDestruct final : Behaviour {
        SelfDestruct(BehaviourScene &scene, int &destroyed) : scene(scene), destroyed(destroyed) {}
        void FixedUpdate(float) override { scene.Destroy(GetEntity()); }
        void OnDestroy() override { ++destroyed; }
        BehaviourScene &scene;
        int &destroyed;
    };
    int destroyed{};
    const auto temporary = scene.Create();
    scene.Attach<SelfDestruct>(temporary, scene, destroyed);
    scene.FixedUpdate(0.1f);
    CHECK(destroyed == 1 && !scene.Components().IsAlive(temporary));
    const auto tree = ui::views::Column("controls", {
        ui::views::Text("title", "Ant"),
        ui::views::Row("actions", {ui::views::Button("play", "Play", [] {})})});
    CHECK(tree.children.size() == 2);

    ui::ValidateView(tree);
    bool rejected = false;
    try { ui::ValidateView(ui::views::Row("root", {ui::views::Text("same", "A"), ui::views::Text("same", "B")})); }
    catch (const std::invalid_argument &) { rejected = true; }
    CHECK(rejected);
    int builds = 0;
    ui::StatefulView<int> counter(0, [&](const int &value) {
        ++builds; return ui::views::Text("count", std::to_string(value));
    });
    CHECK(counter.Build().text == "0");
    counter.Build(); CHECK(builds == 1);
    counter.SetState([](int &value) { ++value; });
    CHECK(counter.Build().text == "1" && builds == 2);
    bool toggled = false;
    auto toggle = ui::views::Toggle("toggle", "Follow", false, [&](bool value) { toggled = value; });
    toggle.onPressed(); CHECK(toggled);
    struct FailingRecipe : EntityArchetype {
        void Build(ecs::World &world, ecs::Entity id) const override {
            world.Add<Health>(id); throw std::runtime_error("construction failure");
        }
    };
    ecs::World recipeWorld;
    try { FailingRecipe{}.Instantiate(recipeWorld); } catch (const std::runtime_error &) {}
    CHECK(recipeWorld.Size() == 0 && recipeWorld.Components<Health>().empty());
    struct Cascade : Behaviour {
        Cascade(BehaviourScene &scene, ecs::Entity other, int &count) : scene(scene), other(other), count(count) {}
        void OnDestroy() override { ++count; scene.Destroy(other); }
        BehaviourScene &scene; ecs::Entity other; int &count;
    };
    const auto first = scene.Create(), second = scene.Create();
    int cascades = 0;
    scene.Attach<Cascade>(first, scene, second, cascades);
    scene.Attach<Cascade>(second, scene, first, cascades);
    scene.Destroy(first);
    CHECK(cascades == 2 && scene.Components().Size() == 0);

    {
        BehaviourScene owner;
        std::vector<int> lifecycle;
        auto object = owner.CreateObject();
        object.AddComponent<Health>();
        object.Attach<Lifecycle>(lifecycle);
        owner.FixedUpdate(0.1f);
        object.SetActive(false); owner.FixedUpdate(0.1f);
        CHECK(object.GetComponent<Health>()->value == 11);
        object.SetActive(true); owner.FixedUpdate(0.1f);
        CHECK(object.GetComponent<Health>()->value == 12);
        object.Destroy();
        CHECK((lifecycle == std::vector<int>{1,2,3,4,1,3,4,5}));
    }
    {
        BehaviourScene hierarchy;
        auto root = hierarchy.CreateObject();
        auto child = hierarchy.CreateObject();
        auto leaf = hierarchy.CreateObject();
        child.SetParent(root); leaf.SetParent(child);
        CHECK(root.GetChildren().size() == 1 && leaf.GetParent().GetEntity() == child.GetEntity());
        bool rejectedCycle = false;
        try { root.SetParent(leaf); } catch (const std::invalid_argument &) { rejectedCycle = true; }
        CHECK(rejectedCycle && !root.GetParent().IsValid());
        BehaviourScene otherScene;
        bool rejectedScene = false;
        try { child.SetParent(otherScene.CreateObject()); } catch (const std::invalid_argument &) { rejectedScene = true; }
        CHECK(rejectedScene);
        root.SetActive(false);
        CHECK(child.IsActiveSelf() && !child.IsActive() && !leaf.IsActive());
        child.DetachFromParent();
        CHECK(child.IsActive() && leaf.IsActive());
        child.SetParent(root);
        std::vector<int> order;
        struct DestroyOrder : Behaviour {
            DestroyOrder(std::vector<int> &order, int value) : order(order), value(value) {}
            void OnDestroy() override { order.push_back(value); }
            std::vector<int> &order; int value;
        };
        root.Attach<DestroyOrder>(order, 1); child.Attach<DestroyOrder>(order, 2); leaf.Attach<DestroyOrder>(order, 3);
        root.Destroy();
        CHECK(!root.IsValid() && !child.IsValid() && !leaf.IsValid());
        CHECK((order == std::vector<int>{3,2,1}));
        auto replacement = hierarchy.CreateObject();
        CHECK(replacement.GetChildren().empty());
    }
    {
        BehaviourScene owner;
        auto object = owner.CreateObject();
        struct DisableOnStart : Behaviour {
            explicit DisableOnStart(int &updates) : updates(updates) {}
            void Start() override { GetObject().SetActive(false); }
            void FixedUpdate(float) override { ++updates; }
            int &updates;
        };
        int updates = 0;
        object.Attach<DisableOnStart>(updates);
        owner.FixedUpdate(0.1f);
        CHECK(updates == 0);
        object.Destroy();
    }
    SceneObject expired;
    {
        BehaviourScene owner;
        auto object = owner.CreateObject();
        object.AddComponent<Health>();
        const auto numericId = object.GetEntity();
        object.Destroy();
        owner.Create(numericId);
        CHECK(!object.IsValid());
        auto replacement = owner.GetObject(numericId);
        replacement.AddComponent<Health>();
        bool staleRejected = false;
        try { object.GetComponent<Health>(); } catch (const std::logic_error &) { staleRejected = true; }
        CHECK(staleRejected && replacement.GetComponent<Health>()->value == 10);
        expired = replacement;
        owner.Clear();
        owner.Create(numericId);
        CHECK(!replacement.IsValid());
        expired = owner.GetObject(numericId);
    }
    CHECK(!expired.IsValid());

    {
        FixedStepSequence phases(2);
        int calls = 0;
        phases.Execute(0, 0.1f, [&] { ++calls; });
        bool duplicate = false;
        try { phases.Execute(0, 0.1f, [&] { ++calls; }); } catch (const std::logic_error &) { duplicate = true; }
        bool changedDelta = false;
        try { phases.Execute(1, 0.2f, [&] { ++calls; }); } catch (const std::invalid_argument &) { changedDelta = true; }
        CHECK(duplicate && changedDelta && calls == 1);
        phases.Execute(1, 0.1f, [&] { ++calls; });
        CHECK(phases.CompletedTicks() == 1 && calls == 2);
        try { phases.Execute(0, 0.1f, [] { throw std::runtime_error("failed system"); }); }
        catch (const std::runtime_error &) {}
        CHECK(phases.IsFaulted());
        bool stopped = false;
        try { phases.Execute(0, 0.1f, [&] { ++calls; }); } catch (const std::logic_error &) { stopped = true; }
        CHECK(stopped && calls == 2);
    }
    {
        struct HookScript : Behaviour {
            explicit HookScript(int &destroyed) : destroyed(destroyed) {}
            void OnDestroy() override { ++destroyed; }
            int &destroyed;
        };
        struct HookRecipe : EntityArchetype {
            explicit HookRecipe(int &destroyed) : destroyed(destroyed) {}
            void Build(ecs::World &world, ecs::Entity id) const override { world.Add<Health>(id); }
            void OnInstantiated(const SceneObject &object) const override {
                object.Attach<HookScript>(destroyed);
                throw std::runtime_error("hook failed");
            }
            int &destroyed;
        };
        int destroyed = 0;
        BehaviourScene owner;
        try { owner.Instantiate(HookRecipe(destroyed)); } catch (const std::runtime_error &) {}
        CHECK(owner.Components().Size() == 0 && destroyed == 1);
    }
    {
        ecs::World world;
        const auto entity = world.Create();
        world.Add<Health>(entity);
        auto access = world.BorrowComponents<Health>();
        access.Get(entity)->value = 7;
        CHECK(world.Get<Health>(entity)->value == 7 && access.Get(999) == nullptr);
        world.Add<int>(entity, 3);
        bool invalidated = false;
        try { access.Get(entity); } catch (const std::logic_error &) { invalidated = true; }
        CHECK(invalidated);
        auto current = world.BorrowComponents<Health>();
        world.Destroy(entity);
        invalidated = false;
        try { current.Get(entity); } catch (const std::logic_error &) { invalidated = true; }
        CHECK(invalidated);
        CHECK(world.BorrowComponents<Health>().Get(entity) == nullptr);
    }
    {
        struct Sensor { int samples{}; };
        struct SensorView : ComponentView<Sensor, Transform2DComponent> {
            using ComponentView::ComponentView;
            ecs::Entity GetId() const { return GetObject().GetEntity(); }
        };
        BehaviourScene scene;
        auto object = scene.CreateObject();
        object.AddComponent<Sensor>();
        object.AddComponent<Transform2DComponent>();
        SensorView view(object);
        view.Require<Sensor>().samples = 7;
        const auto id = object.GetEntity();
        // Force dense pool growth and verify cached pointers refresh.
        for (int i = 0; i < 128; ++i) {
            auto other = scene.CreateObject();
            other.AddComponent<Sensor>();
            other.AddComponent<Transform2DComponent>();
        }
        CHECK(view.Require<Sensor>().samples == 7);
        SceneViewCache<SensorView, Sensor> query(scene);
        CHECK(query.Size() == 129 && query.Find(id));
        object.Destroy();
        CHECK(query.Find(id) == nullptr);
        bool rejected = false;
        try { (void)view.Require<Sensor>(); } catch (const std::logic_error &) { rejected = true; }
        CHECK(rejected);
        scene.Components().Create(id);
        scene.Components().Add<Sensor>(id);
        rejected = false;
        try { (void)view.Require<Sensor>(); } catch (const std::logic_error &) { rejected = true; }
        CHECK(rejected);
        DirectionTracker tracker;
        tracker.SetAngleInstant(0);
        tracker.SetTarget({0, 1});
        tracker.Update(0.1f);
        CHECK(tracker.GetAngle() > 0);
        CHECK((SeekVelocity({}, {}, {3, 4}, 10) == Vector2f{6, 8}));
    }
    constexpr Vector2f sum = Vector2f{1.0f, 2.0f} + Vector2f{3.0f, 4.0f};
    static_assert(sum == Vector2f{4.0f, 6.0f});
    static_assert(Rectanglei{{0, 0}, {10, 10}}.Contains({9, 9}));
    static_assert(!Rectanglei{{0, 0}, {10, 10}}.Contains({10, 9}));
    static_assert(Angle::FromDegrees(180.0f).Radians() > 3.14f);
    static_assert(TimeSpan::FromMilliseconds(250.0).Seconds() == 0.25);
    static_assert(std::is_same_v<std::variant_alternative_t<4, PropertyValue>, Vector2f>);
    static_assert(std::is_same_v<decltype(NetworkNodeViewModel::color), Color>);
    GeometryCommand command;
    command.vertices.push_back({{1.0f, 2.0f}, {255, 0, 0, 255}, {0.0f, 0.0f}});
    CHECK(command.vertices.size() == 1);
    const InputEvent event{InputEventType::PointerMoved, PointerMoveInput{{12, 34}}};
    CHECK((event.GetIf<PointerMoveInput>()->position == Vector2i{12, 34}));
}
