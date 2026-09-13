#include <PipeFrame/Simulation/SimulationController.h>
#include <PipeFrame/Backend/SFML/UI/Bindings.h>
#include <PipeFrame/Backend/SFML/UI/Motion.h>
#include <PipeFrame/Backend/SFML/SimulationDashboardHost.h>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <cmath>
#include <iostream>

bool Check(bool value, const char *message) {
    if (!value)
        std::cerr << "FAILED: " << message << '\n';
    return value;
}
class CountedPanel : public Panel {
  public:
    unsigned changes = 0;

  protected:
    void OnGeometryChanged() override {
        ++changes;
        Panel::OnGeometryChanged();
    }
};
class CountingPresenter : public pipeframe::ui::ViewPanel {
public:
    explicit CountingPresenter(unsigned &builds) : builds(builds) { SetPreferredSize({200,100}); }
protected:
    pipeframe::ui::View BuildView() override {
        ++builds;
        return pipeframe::ui::views::Text("value",std::to_string(builds));
    }
private:
    unsigned &builds;
};
int main() {
    bool passed = true;
    Column layout;
    layout.SetSize({300, 200});
    auto &first = layout.CreateChild<CountedPanel>();
    first.SetSize({0, 40});
    auto &second = layout.CreateChild<CountedPanel>();
    second.SetSize({0, 50});
    layout.Arrange({{}, {300, 200}});
    const auto firstCount = first.changes, secondCount = second.changes;
    for (int i = 0; i < 100; ++i)
        layout.Arrange({{}, {300, 200}});
    passed &= Check(first.changes == firstCount && second.changes == secondCount,
                    "Identical arrangements must not invalidate retained geometry");
    first.SetVisible(false);
    passed &= Check(second.GetPosition().y == 0, "Visibility invalidation must reflow siblings");
    first.SetVisible(true);
    first.SetSize({0, 65});
    passed &= Check(second.GetPosition().y == 65, "Size invalidation must reflow siblings");
    using namespace pipeframe::ui;
    AnimatedFloat one(0), partitioned(0);
    one.SetTarget(1);
    partitioned.SetTarget(1);
    one.Update(0.2f, 0.4f);
    partitioned.Update(0.1f, 0.4f);
    partitioned.Update(0.1f, 0.4f);
    passed &= Check(std::abs(one.Get() - partitioned.Get()) < 1e-6f,
                    "Animation clock must be invariant to frame partitioning");
    const float previous = one.Get();
    one.Update(-1, 0.4f);
    passed &= Check(one.Get() == previous, "Negative clock deltas must not rewind motion");
    SimulationController simulation;
    simulation.Pause();
    one.Update(0.2f, 0.4f);
    passed &= Check(one.Get() == 1 && simulation.GetTickCount() == 0,
                    "Real UI motion must complete while simulation is paused");
    State<bool> visible{true};
    {
        Panel bound;
        BindVisible(bound, visible);
        visible.Set(false);
        passed &= Check(!bound.IsVisible(), "Live state binding must update visibility");
    }
    visible.Set(true);
    passed &= Check(visible.GetObserverCount() == 0, "Destroyed controls must release bindings");
    sf::Font font;
    if (!font.openFromFile(PIPEFRAME_HARDENING_FONT))
        return 1;
    Label wrapping(font);
    wrapping.SetWrap(true);
    wrapping.SetText("A long status message that wraps when the panel becomes narrow.");
    const auto drawLabel = [&](float width) {
        wrapping.SetSize({width, 140});
        sf::RenderTexture target({320, 160});
        target.clear(sf::Color::Black);
        wrapping.Render(target);
        target.display();
        return target.getTexture().copyToImage();
    };
    const auto narrow = drawLabel(80);
    const auto wide = drawLabel(300);
    const auto restored = drawLabel(80);
    bool same = true, different = false;
    for (unsigned y = 0; y < 160; ++y)
        for (unsigned x = 0; x < 320; ++x) {
            same &= narrow.getPixel({x, y}) == restored.getPixel({x, y});
            different |= narrow.getPixel({x, y}) != wide.getPixel({x, y});
        }
    passed &= Check(same && different,
                    "Wrapped text cache must invalidate on width changes and restore the original rendering");
    unsigned builds=0;
    std::weak_ptr<void> panelLifetime;
    {
        pipeframe::backend::sfml::HostedViewPanel<CountingPresenter> host(font,builds);
        panelLifetime=host.Lifetime();
        host.Update(.016f);
        const auto initialBuilds=builds;
        passed &= Check(initialBuilds>0 && host.GetSize()==sf::Vector2f{200,100},
                        "Neutral presenter mounts in the shared host with preferred dimensions");
        for(int i=0;i<10;++i) host.Update(.016f);
        passed &= Check(builds==initialBuilds,"Clean presenter is not rebuilt on every frame");
        host.InvalidateView();host.InvalidateView();host.InvalidateView();host.Update(.016f);
        passed &= Check(builds==initialBuilds+1,"Repeated presenter invalidations coalesce into one rebuild");
    }
    passed &= Check(panelLifetime.expired(),"Hosted presenter releases native widget lifetime on teardown");
    NativeSimulationDashboard dashboard(font, "Focus regression");
    auto &page = dashboard.AddPage("TOOLS");
    int actions = 0;
    dashboard.Action(page, "ACTION", [&] { ++actions; });
    dashboard.Layout({{30, 40}, {640, 600}});
    dashboard.Drawer().Close(false);
    dashboard.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Tab});
    dashboard.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Enter});
    dashboard.HandleEvent(sf::Event::KeyReleased{sf::Keyboard::Key::Enter});
    passed &= Check(actions == 0, "A closed drawer must exclude hidden page actions from keyboard traversal");
    dashboard.SetVisible(false);
    passed &= Check(!dashboard.Contains({50, 100}) && !dashboard.HasKeyboardFocus(),
                    "Hidden dashboards must release their visible input contract");
    if (passed)
        std::cout << "UI hardening regression passed.\n";
    return passed ? 0 : 1;
}
