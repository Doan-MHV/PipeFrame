#pragma once
#include <PipeFrame/Project/SceneProjectRuntime.h>
#include <PipeFrame/UI/ViewPanel.h>
#include <PipeFrame/UI/StatefulView.h>
namespace counter_example {
struct CounterComponent {
    double rate{1};
    static auto Schema() {
        return pipeframe::ComponentSchema<CounterComponent>("counter.settings", "Counter")
            .Editable({.key="rate", .displayName="Rate", .kind=pipeframe::PropertyKind::Number,
                       .defaultValue=1.0, .minimum=0, .maximum=10}, &CounterComponent::rate);
    }
};
class CounterBehaviour final : public pipeframe::Behaviour {
    void FixedUpdate(float delta) override {
        auto *settings=GetComponent<CounterComponent>();
        auto *pose=GetComponent<pipeframe::Transform2DComponent>();
        if(settings && pose) pose->position.x+=static_cast<float>(settings->rate)*delta;
    }
};
class CounterEntity final : public pipeframe::EntityArchetype {
    void Build(pipeframe::ecs::World &world, pipeframe::ecs::Entity entity) const override {
        world.Add<pipeframe::Transform2DComponent>(entity);
        world.Add<CounterComponent>(entity);
    }
    void OnInstantiated(const pipeframe::SceneObject &object) const override { object.Attach<CounterBehaviour>(); }
};
class CounterRuntime final : public pipeframe::SceneProjectRuntime {
public:
    CounterRuntime() : SceneProjectRuntime("Independent counter") {
        RegisterComponent<CounterComponent>();
        RegisterEntity<CounterEntity>({"counter.entity", "Counter", {}, {}});
    }
};
// Project code describes content; the host supplies native resources and event routing.
class CounterPanel final : public pipeframe::ui::ViewPanel {
public:
    explicit CounterPanel(pipeframe::SceneObject object)
        : clicks(0,[object](const int &count,const pipeframe::ui::StatefulView<int>::Setter &set) {
            using namespace pipeframe::ui;
            return views::Column("content",{
                views::Card("card","Counter controller", "Changes: "+std::to_string(count)),
                views::Button("faster","Increase rate",[object,set] {
                    if(!object.IsValid())return;
                    if(auto *component=object.GetComponent<CounterComponent>()) {
                        component->rate=std::min(10.0,component->rate+1);
                        set([](int &value){++value;});
                    }
                }),
                views::Text("help","The attached Behaviour consumes the same component on the next fixed step.").FitHeight()
            });
        }) {}
    int Changes() const { return clicks.GetState(); }
protected:
    pipeframe::ui::View BuildView() override {
        using namespace pipeframe::ui;
        return views::Scroll("scroll",views::Column("panel",{clicks.Describe("counter")})).FillHeight();
    }
private:
    pipeframe::ui::StatefulView<int> clicks;
};
}
