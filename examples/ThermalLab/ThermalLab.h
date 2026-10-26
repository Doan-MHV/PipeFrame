#ifndef PIPEFRAME_THERMAL_LAB_H
#define PIPEFRAME_THERMAL_LAB_H
#include <PipeFrame/Simulation/SimulationController.h>
#include <PipeFrame/Backend/SFML/UI/Bindings.h>
#include <PipeFrame/Backend/SFML/UI/Chart.h>
#include <PipeFrame/Backend/SFML/UI/Component.h>
#include <PipeFrame/Backend/SFML/UI/SimulationTransport.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <cmath>

// A complete application composed exclusively from public PipeFrame facilities.
class ThermalLab {
  public:
    explicit ThermalLab(const sf::Font &font) {
        using namespace pipeframe::ui;
        controller.Pause();
        root = &ui.CreateRoot<Column>();
        root->SetFillColor(UITheme::Dark().applicationBackground);
        root->SetPadding(Thickness{20});
        root->SetSpacing(12);
        auto &title = root->CreateChild<Label>(font);
        title.SetText("Thermal lab");
        title.SetCharacterSize(26);
        title.SetSize({0, 40});
        auto &subtitle = root->CreateChild<Label>(font);
        subtitle.SetText("A public-API example: state, layout, plots and transport");
        subtitle.SetWrap(true);
        subtitle.SetSize({0, 36});
        Compose(*root, Component<StackPanel>("body", [](StackPanel &panel) {
            Clear(panel);
            panel.SetSpacing(16);
        }));
        body = static_cast<StackPanel *>(root->FindChildByKey("body"));
        root->SetChildFlex(*body, 1);
        controls = &body->CreateChild<Column>();
        Clear(*controls);
        controls->SetSpacing(12);
        auto &card = controls->CreateChild<MetricCard>(font);
        card.SetTitle("TEMPERATURE");
        card.SetSize({0, 88});
        BindValueText(card, temperatureText);
        card.SetDetail("Degrees Celsius / synthetic sensor");
        auto &targetLabel = controls->CreateChild<Label>(font);
        targetLabel.SetSize({0, 28});
        BindText(targetLabel, targetText);
        slider = &controls->CreateChild<Slider>();
        slider->SetSize({0, 32});
        slider->SetRange(0, 100);
        slider->SetStep(1);
        BindValue(*slider, target);
        slider->SetOnValueChanged([this](float value) {
            target.Set(value);
            targetText.Set("Target: " + std::to_string(static_cast<int>(value)) + " C");
        });
        progress = &controls->CreateChild<ProgressBar>();
        progress->SetSize({0, 16});
        progress->SetValue(0.42f, false);
        chart = &body->CreateChild<TimeSeriesChart>();
        body->SetChildFlex(*chart, 1);
        chart->SetVerticalRange(ChartRange{0, 100});
        transport = &root->CreateChild<SimulationTransport>(font);
        transport->SetOnPlayPause([this] {
            controller.TogglePlayPause();
            RefreshTransport();
        });
        transport->SetOnSingleStep([this] {
            controller.RequestSingleStep();
            Step(1.f / 60);
        });
        transport->SetOnReset([this] {
            controller.Pause();
            controller.ResetTickCount();
            temperature = 42;
            ResetHistory();
            RefreshTransport();
        });
        transport->SetOnSpeedSelected([this](auto speed) {
            controller.SetSpeed(speed);
            RefreshTransport();
        });
        ResetHistory();
        RefreshTransport();
    }
    void Layout(sf::Vector2f size) {
        const bool narrow = size.x < 760;
        body->SetOrientation(narrow ? StackOrientation::Vertical : StackOrientation::Horizontal);
        controls->SetSize(narrow ? sf::Vector2f{0, 216} : sf::Vector2f{290, 0});
        controls->SetSizePolicy(narrow ? SizePolicy::Stretch : SizePolicy::Fixed,
                                narrow ? SizePolicy::Fixed : SizePolicy::Stretch);
        root->Arrange({{}, size});
    }
    bool HandleEvent(const sf::Event &event) { return ui.HandleEvent(event); }
    void Update(float realSeconds) {
        ui.Update(realSeconds);
        if (controller.IsPlaying())
            Step(realSeconds);
    }
    void Render(sf::RenderTarget &target) const { ui.Render(target); }
    Widget &Root() { return *root; }
    Slider &TargetSlider() { return *slider; }
    SimulationTransport &Transport() { return *transport; }
    float GetTarget() const { return target.Get(); }
    std::uint64_t GetTicks() const { return controller.GetTickCount(); }

  private:
    static void Clear(Panel &panel) {
        panel.SetFillColor(sf::Color::Transparent);
        panel.SetOutlineThickness(0);
    }
    void RefreshTransport() { transport->SetSimulationState(controller.IsPlaying(), true, controller.GetSpeed()); }
    void Step(float dt) {
        if (!controller.ConsumeTick())
            return;
        temperature += (target.Get() - temperature) * std::min(1.f, std::max(0.f, dt) * controller.GetTimeScale());
        elapsed += dt;
        samples.push_back({elapsed, temperature});
        if (samples.size() > 120)
            samples.erase(samples.begin());
        temperatureText.Set(std::to_string(static_cast<int>(std::round(temperature))) + " C");
        progress->SetValue(temperature / 100, false);
        chart->SetSeries({{samples}});
    }
    void ResetHistory() {
        elapsed = 20;
        samples.clear();
        for (int i = 0; i <= 20; ++i)
            samples.push_back({static_cast<float>(i), 22.f + static_cast<float>(i)});
        temperatureText.Set("42 C");
        progress->SetValue(0.42f, false);
        chart->SetSeries({{samples}});
    }
    pipeframe::ui::State<float> target{70};
    pipeframe::ui::State<std::string> temperatureText{"42 C"}, targetText{"Target: 70 C"};
    UIManager ui;
    SimulationController controller;
    Column *root, *controls;
    StackPanel *body;
    Slider *slider;
    ProgressBar *progress;
    TimeSeriesChart *chart;
    SimulationTransport *transport;
    float temperature = 42, elapsed = 20;
    std::vector<sf::Vector2f> samples;
};
#endif
