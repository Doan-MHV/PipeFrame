#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <PipeFrame/UI/StatefulView.h>
#include <PipeFrame/UI/SchemaInspector.h>
#include <PipeFrame/UI/State.h>
#include <PipeFrame/Backend/SFML/UI/TextField.h>
#include <PipeFrame/Backend/SFML/UI/TextButton.h>
#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/ScrollPanel.h>
#include <PipeFrame/Backend/SFML/UI/Chart.h>
#include <PipeFrame/Backend/SFML/UI/ProgressBar.h>
#include <PipeFrame/Project/ComponentSchema.h>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <iostream>
#include <stdexcept>

using namespace pipeframe::ui;
namespace {
void Check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
Widget *Find(Widget *root,const std::string &key) {
    if (!root) return nullptr;
    if (root->GetKey()==key) return root;
    for (std::size_t i=0;i<root->GetChildCount();++i) if (auto *found=Find(root->GetChild(i),key)) return found;
    return nullptr;
}
Widget &Find(UIManager &ui,const std::string &key) {
    for (std::size_t i=0;i<ui.GetRootCount();++i) if (auto *found=Find(ui.GetRoot(i),key)) return *found;
    throw std::runtime_error("Missing widget: "+key);
}
void Click(UIManager &ui,Widget &widget) {
    auto position=widget.GetScreenPosition()+sf::Vector2f{5,5};
    sf::Vector2i point{int(position.x),int(position.y)};
    ui.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point});
    ui.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,point});
}
void Load(UIManager &ui) { Check(ui.LoadDefaultFont(PIPEFRAME_TEST_FONT),"Test font loads"); }
void TestCleanMountSkipsDescriptionTraversal() {
    struct CountingSource final : ViewSource {
        int builds{};
        View view=views::Text("value","A wrapped label whose measurements must remain valid on resize.").FitHeight();
        const View &Build() override { ++builds; return view; }
        std::uint64_t Revision() const override { return 1; }
        void Mount() override {}
        void Unmount() noexcept override {}
    };
    UIManager ui; Load(ui);
    auto source=std::make_shared<CountingSource>();
    auto mount=ui.MountView(views::Stateful("source",source));
    mount.SetBounds(0,0,300,200); ui.Update(0);
    const auto height=Find(ui,"value").GetSize().y;
    for(int i=0;i<100;++i) ui.Update(.016f);
    Check(source->builds==1,"Clean frames must not resolve cached source descriptions");
    mount.SetBounds(0,0,100,200); ui.Update(0);
    Check(source->builds==1 && Find(ui,"value").GetSize().y>height,
        "A clean mount must still remeasure wrapped text on resize");
}
void TestMountedStateAndDisposal() {
    UIManager ui; Load(ui);
    int childBuilds{},parentBuilds{},mounts{},unmounts{};
    StatefulView<int>::Setter stale;
    StatefulView<int> child(0,[&](const int &value,const StatefulView<int>::Setter &set) {
        ++childBuilds; stale=set;
        return views::Button("increment",std::to_string(value),[set] { set([](int &v) { ++v; }); });
    });
    State<int> external(0);
    child.SetLifecycle([&] { ++mounts; child.RetainForMount(external.Observe([](int) {})); },[&] { ++unmounts; });
    StatefulView<bool> parent(true,[&](const bool &show) {
        ++parentBuilds;
        return views::Column("parent",show ? std::vector<View>{child.Describe("child")} : std::vector<View>{});
    });
    auto mount=ui.MountView(parent.Describe("root")); mount.SetBounds(0,0,300,200); ui.Update(0);
    Check(external.GetObserverCount()==1 && mounts==1 && childBuilds==1 && parentBuilds==1,"Initial mounted lifecycle");
    auto *button=&Find(ui,"increment");
    Click(ui,*button); ui.Update(0);
    Check(child.GetState()==1 && childBuilds==2 && parentBuilds==1,"Nested child rebuild independent of parent");
    Check(&Find(ui,"increment")==button && button->HasKeyboardFocus(),"Rebuild retains keyed control and focus");
    ui.Update(0); Check(childBuilds==2,"Clean state does not rerun builder");
    const auto buttonLifetime=button->Lifetime();
    parent.SetState([](bool &show) { show=false; }); ui.Update(0);
    Check(external.GetObserverCount()==0 && unmounts==1 && buttonLifetime.expired() && !ui.HasKeyboardFocus(),"Removed source disposes widget and focus");
    stale([](int &v) { v=99; }); Check(child.GetState()==1,"Disposed setter cannot mutate state");
    parent.SetState([](bool &show) { show=true; }); ui.Update(0);
    Check(mounts==2 && child.IsMounted(),"Source can be remounted");
    mount.Unmount(); ui.Update(0);
    Check(ui.GetRootCount()==0 && !parent.IsMounted() && !child.IsMounted() && unmounts==2,"Unmount destroys root and nested sources");
}
void TestReconcileFocusCaptureAndOrder() {
    UIManager ui; Load(ui); int commits{},clicks{};
    auto make=[&](bool reverse) {
        auto input=views::Input("input","initial",[&](const std::string &) { ++commits; });
        auto button=views::Button("button","Action",[&] { ++clicks; });
        return views::Column("root",reverse ? std::vector<View>{button,input} : std::vector<View>{input,button});
    };
    auto mount=ui.MountView(make(false)); mount.SetBounds(0,0,300,200); ui.Update(0);
    auto &input=Find(ui,"input"); const auto lifetime=input.Lifetime();
    Click(ui,input); ui.HandleEvent(sf::Event::TextEntered{U'z'});
    mount.SetView(make(true)); ui.Update(0);
    Check(&input==&Find(ui,"input") && input.HasKeyboardFocus(),"Reorder preserves text editing focus");
    Check(Find(ui,"button").GetPosition().y < input.GetPosition().y,"New description order changes layout");
    ui.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Enter});
    Check(commits==1 && dynamic_cast<TextField &>(input).GetValue()=="z","Rebuild preserves uncommitted edit buffer");
    const auto point=Find(ui,"button").GetScreenPosition()+sf::Vector2f{5,5};
    ui.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,{int(point.x),int(point.y)}});
    mount.SetView(views::Column("root",{views::Text("button","Replacement"),views::Text("input","Replaced input")}));
    ui.Update(0);
    ui.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,{int(point.x),int(point.y)}});
    Check(lifetime.expired() && clicks==0,"Type replacement releases old capture and input");
    auto held=std::make_shared<int>(1); std::weak_ptr<int> resource=held;
    mount.SetView(views::Button("resource","Resource",[held] {})); held.reset(); ui.Update(0);
    mount.SetView(views::Text("resource","Removed")); ui.Update(0);
    Check(resource.expired(),"Removed callback releases captured resources");
    mount.SetView(views::Button("disabled","Disabled",[&] { ++clicks; }).Enabled(false)); ui.Update(0);
    Click(ui,Find(ui,"disabled")); Check(clicks==0,"Disabled descriptions cannot activate");
}
void TestNestedScrollBubblesAtEdges() {
    UIManager ui; Load(ui);
    auto make=[](bool overflow) {
        return views::Scroll("outer",views::Column("page",{
            views::Scroll("inner",views::Column("list",{
                views::Button("item","One item",[]{}),
                views::Text("tail","").Height(overflow?300:0)
            })).Height(140),
            views::Button("outside","Outside list",[]{}),
            views::Text("long-page","").Height(600)
        }));
    };
    auto mount=ui.MountView(make(false));mount.SetBounds(0,0,320,250);ui.Update(0);
    auto &outer=dynamic_cast<ScrollPanel&>(Find(ui,"outer"));
    auto &inner=dynamic_cast<ScrollPanel&>(Find(ui,"inner"));
    auto wheel=[&](float delta,sf::Vector2i point){ui.HandleEvent(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical,delta,point});};
    wheel(-1,{20,100});
    Check(outer.GetScrollOffset()>0&&inner.GetScrollOffset()==0,
          "Empty space in a short nested asset list must scroll the surrounding page");
    outer.SetScrollOffset(0);wheel(-1,{20,15});
    Check(outer.GetScrollOffset()>0,"A short list's button must also bubble wheel events");
    outer.SetScrollOffset(0);mount.SetView(make(true));ui.Update(0);
    wheel(-1,{20,100});
    Check(inner.GetScrollOffset()>0&&outer.GetScrollOffset()==0,"Overflowing inner list scrolls first");
    inner.SetScrollOffset(inner.GetMaximumScrollOffset());wheel(-1,{20,100});
    Check(outer.GetScrollOffset()>0,"At the inner bottom, downward scrolling reaches the parent");
    inner.SetScrollOffset(0);wheel(1,{20,60});
    Check(outer.GetScrollOffset()==0,"At the inner top, upward scrolling reaches the parent");
}
void TestLayoutScrollPopupAndFailure() {
    UIManager ui; Load(ui); int clicks{};
    auto longText=views::Text("long","A long wrapped Inspector explanation that must grow vertically when the dock is narrow and remain inside its assigned width.").FitHeight();
    auto content=views::Column("content",{longText,views::Button("action","Action",[&] { ++clicks; }),views::Text("filler","Filler").Height(500)});
    auto mount=ui.MountView(views::Scroll("scroll",content)); mount.SetBounds(0,0,320,180); ui.Update(0);
    const auto wide=Find(ui,"long").GetSize().y;
    mount.SetBounds(0,0,160,180); ui.Update(0);
    Check(Find(ui,"long").GetSize().y > wide,"Narrower constraint increases wrapped text height");
    auto &scroll=dynamic_cast<ScrollPanel &>(Find(ui,"scroll"));
    ui.HandleEvent(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical,-3,{30,30}});
    Check(scroll.GetScrollOffset()>0,"Mounted scroll owns wheel events");
    const auto offset=scroll.GetScrollOffset();
    mount.SetView(views::Scroll("scroll",content)); ui.Update(0);
    Check(scroll.GetScrollOffset()==offset,"Rebuild retains scroll offset");
    auto bad=views::Column("bad",{views::Text("duplicate","a"),views::Text("duplicate","b")});
    bool rejected=false; try { mount.SetView(bad); } catch(const std::invalid_argument &) { rejected=true; }
    Check(rejected && &scroll==&Find(ui,"scroll"),"Invalid description leaves mounted tree intact");
    mount.SetBounds(0,0,400,300);
    mount.SetView(views::Stack("layers",{
        views::Button("background","Background",[&] { ++clicks; }),
        views::Popup("modal",{views::Column("dialog",{views::Input("modal-input","",[](const auto &) {})}).Width(220)})}));
    ui.Update(0);
    ui.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,{5,5}});
    ui.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,{5,5}});
    Check(clicks==0,"Popup prevents background activation");
    ui.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Tab});
    Check(Find(ui,"modal-input").HasKeyboardFocus(),"Keyboard traversal stays in popup");
    mount.SetView(views::Button("background","Background",[&] { ++clicks; })); ui.Update(0);
    Click(ui,Find(ui,"background")); Check(clicks==1,"Removing popup restores background input");
}
void TestBuildFailureAndHostTeardown() {
    int unmounts{};
    StatefulView<int> state(0,[](const int &value) {
        if (value==1) throw std::runtime_error("deliberate builder failure");
        return views::Text("value",std::to_string(value));
    });
    state.SetLifecycle([] {},[&] { ++unmounts; });
    MountedView handle;
    {
        UIManager ui; Load(ui);
        handle=ui.MountView(state.Describe("state")); handle.SetBounds(0,0,300,100); ui.Update(0);
        auto &label=Find(ui,"value");
        state.SetState([](int &value) { value=1; });
        bool rejected=false; try { ui.Update(0); } catch(const std::runtime_error &) { rejected=true; }
        Check(rejected && &label==&Find(ui,"value") && dynamic_cast<Label &>(label).GetText()=="0",
              "Failed builder leaves previous tree intact");
        state.SetState([](int &value) { value=2; }); ui.Update(0);
        Check(dynamic_cast<Label &>(Find(ui,"value")).GetText()=="2","Failed build can recover");
        handle.SetView(views::Column("duplicate-source",{state.Describe("first"),state.Describe("second")}));
        rejected=false; try { ui.Update(0); } catch(const std::invalid_argument &) { rejected=true; }
        Check(rejected && state.IsMounted(),"Duplicate source rejected without destroying old mount");
    }
    Check(!handle.IsMounted() && !state.IsMounted() && unmounts==1,"UI host teardown disposes mount");
}
void TestMountDuringLifecycle() {
    UIManager ui; Load(ui); MountedView second;
    StatefulView<int> first(0,[](const int &) { return views::Text("first-text","First"); });
    first.SetLifecycle([&] {
        second=ui.MountView(views::Text("second-text","Second")); second.SetBounds(0,100,300,100);
    },[] {});
    auto mount=ui.MountView(first.Describe("first")); mount.SetBounds(0,0,300,100);
    ui.Update(0); ui.Update(0);
    Check(ui.GetRootCount()==2 && Find(ui,"second-text").IsVisible(),"Mounts created by lifecycle callbacks join next frame safely");
}
void TestSchemaInspector(const char *screenshot) {
    struct Settings { float speed=2; bool enabled=true; pipeframe::Vector2f position{12,24}; } model;
    pipeframe::ComponentSchema<Settings> schema("test.settings","Robot movement");
    pipeframe::PropertyDescriptor speed{"speed","Movement speed",pipeframe::PropertyKind::Number,2.0}; speed.minimum=0; speed.maximum=10; speed.unit="m/s";
    schema.Field(speed,&Settings::speed)
        .Field({"enabled","Simulation enabled",pipeframe::PropertyKind::Boolean,true},&Settings::enabled)
        .Field({"position","Position",pipeframe::PropertyKind::Vector2,pipeframe::Vector2f{}},&Settings::position);
    int errors{};
    UIManager ui; Load(ui);
    StatefulView<int> inspector(0,[&](const int &,const StatefulView<int>::Setter &set) {
        return views::Scroll("inspector-scroll",SchemaInspector("inspector",schema.Describe(),schema.Serialize(model).properties,
            [&,set](const std::string &key,const pipeframe::PropertyValue &value) {
                std::string error; Check(schema.Apply(model,{{key,value}},error),"Schema applies validated edit");
                set([](int &version) { ++version; });
            },[&](const auto &,const auto &) { ++errors; })).FillHeight();
    });
    auto mount=ui.MountView(inspector.Describe("root").FillHeight()); mount.SetBounds(0,0,340,520); ui.Update(0);
    auto &field=*Find(Find(ui,"speed").GetChild(1),"value");
    Click(ui,field); ui.HandleEvent(sf::Event::TextEntered{U'7'}); ui.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Enter}); ui.Update(0);
    Check(model.speed==7,"Schema generated field commits to component");
    Click(ui,field); ui.HandleEvent(sf::Event::TextEntered{U'-'}); ui.HandleEvent(sf::Event::TextEntered{U'1'});
    ui.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Enter}); ui.Update(0);
    Check(model.speed==7 && errors==1,"Out-of-range edit rejected without changing component");
    auto description=SchemaInspector("headless",schema.Describe(),schema.Serialize(model).properties,{},[&](const auto &,const auto &) { ++errors; });
    description.children[1].children[1].onCommitted("nan");
    Check(errors==2,"Nonfinite numeric input rejected");
    if (screenshot) {
        // Reset invalid local editing display before saving the fixture.
        ui.HandleEvent(sf::Event::FocusLost{}); inspector.SetState([](int &v) { ++v; }); ui.Update(0);
        sf::RenderTexture target({340,520}); target.clear(sf::Color{24,25,28}); ui.Render(target); target.display();
        Check(target.getTexture().copyToImage().saveToFile(screenshot),"Save mounted Inspector evidence");
    }
}
void TestResponsiveControls() {
    UIManager ui; Load(ui); int clicks=0;
    const auto describe=[&](float progress) {
        std::vector<View> actions;
        for(int i=0;i<6;++i) actions.push_back(views::Button("action:"+std::to_string(i),"ACTION "+std::to_string(i),[&]{++clicks;}));
        View::Series line; line.samples={{0,1},{1,3},{2,progress*10}};
        return views::Scroll("responsive-scroll",views::Column("content",{
            views::Wrap("actions",std::move(actions),100),views::Progress("progress",progress),views::Chart("chart",{line})
        })).FillHeight();
    };
    auto mount=ui.MountView(describe(.25f)); mount.SetBounds(0,0,240,340); ui.Update(0);
    auto &first=Find(ui,"action:0"); auto &third=Find(ui,"action:2");
    Check(third.GetScreenPosition().y>first.GetScreenPosition().y,"Narrow actions wrap to another row");
    for(int i=0;i<6;++i) {
        auto &button=Find(ui,"action:"+std::to_string(i));
        Check(button.GetSize().x>=100 && button.GetBounds().position.x+button.GetSize().x<=241,"Wrapped hit region stays inside available width");
        Click(ui,button);
    }
    Check(clicks==6,"Every wrapped action is clickable");
    Check(dynamic_cast<ProgressBar &>(Find(ui,"progress")).GetValue()==.25f,"Progress view receives model data");
    mount.SetView(describe(.75f)); mount.SetBounds(0,0,720,340); ui.Update(0);
    Check(Find(ui,"action:5").GetScreenPosition().y==Find(ui,"action:0").GetScreenPosition().y,"Wide actions share one row");
    Check(dynamic_cast<TimeSeriesChart &>(Find(ui,"chart")).GetSeries()[0].samples.back().y==7.5f,"Chart rebuild uses current series");
    sf::RenderTexture target({720,340}); ui.Render(target);
}

}
int main(int argc,char **argv) {
    try {
        TestMountedStateAndDisposal(); TestReconcileFocusCaptureAndOrder(); TestLayoutScrollPopupAndFailure(); TestNestedScrollBubblesAtEdges();
        TestCleanMountSkipsDescriptionTraversal(); TestBuildFailureAndHostTeardown(); TestMountDuringLifecycle(); TestResponsiveControls();
        TestSchemaInspector(argc>1 ? argv[1] : "r4-inspector.png");
        std::cout << "Mounted view lifecycle, state, reconciliation, layout, input and schema tests passed.\n";
    } catch(const std::exception &error) { std::cerr << "FAILED: " << error.what() << '\n'; return 1; }
}
