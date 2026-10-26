#include <PipeFrame/Backend/SFML/UI/Chart.h>
#include <PipeFrame/Backend/SFML/UI/Component.h>
#include <PipeFrame/Backend/SFML/UI/Gauge.h>
#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/MetricCard.h>
#include <PipeFrame/Backend/SFML/UI/NetworkView.h>
#include <PipeFrame/Backend/SFML/UI/TabView.h>
#include <PipeFrame/Backend/SFML/UI/TableView.h>
#include <PipeFrame/Backend/SFML/UI/ProgressBar.h>
#include <PipeFrame/Backend/SFML/UI/Slider.h>
#include <PipeFrame/Backend/SFML/UI/StackPanel.h>
#include <PipeFrame/Backend/SFML/UI/Toggle.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Clock.hpp>
#include <cmath>
#include <iostream>
#include "ControlsGallery.h"

namespace {
void Transparent(StackPanel &panel) {
    panel.SetFillColor(sf::Color::Transparent);
    panel.SetOutlineThickness(0);
    panel.SetHitTestVisible(false);
    panel.SetSpacing(12);
}
Label &Text(Widget &parent, const sf::Font &font, const std::string &text, unsigned int size = 14) {
    auto &label = parent.CreateChild<Label>(font);
    label.SetText(text);
    label.SetCharacterSize(size);
    label.SetSize({0, static_cast<float>(size + 12)});
    return label;
}

class Gallery {
  public:
    explicit Gallery(const sf::Font &font) {
        root = &ui.CreateRoot<Column>();
        Transparent(*root);
        root->SetPadding(Thickness{24});
        Text(*root, font, "PipeFrame / UI gallery", 26);
        Text(*root, font, "Monitoring components - synthetic data, shared engine widgets");
        tabs = &root->CreateChild<TabView>();
        root->SetChildFlex(*tabs, 1);
        plots = &tabs->AddPage<StackPanel>();
        Transparent(*plots);
        auto &networkPage = tabs->AddPage<Column>();
        Transparent(networkPage);
        auto &tablePage = tabs->AddPage<Column>();
        Transparent(tablePage);
        tabs->AddPage<Column>();
        tabs->SetOnSelectionChanged([this](std::size_t index) {
            if (index == 3 && onControls) { tabs->SetSelectedIndex(0); onControls(); }
        });
        for (std::size_t i = 0; i < 4; ++i) {
            auto &caption = Text(*tabs->GetTab(i), font, i == 0 ? "Charts" : (i == 1 ? "Network" : (i==2 ? "Table" : "Controls")));
            caption.SetSize({120, 28});
        }
        Text(networkPage, font, "Directed network", 18);
        Text(networkPage, font, "Click a node to inspect / positions and colors supplied by the application");
        network = &networkPage.CreateChild<NetworkView>();
        networkPage.SetChildFlex(*network, 1);
        network->SetNodeRadius(12);
        std::vector<NetworkNode> nodes;
        std::vector<NetworkEdge> edges;
        for (std::uint64_t layer = 0; layer < 3; ++layer) {
            for (std::uint64_t row = 0; row < 3; ++row) {
                nodes.push_back({layer * 3 + row, {layer * 0.5f, row * 0.5f},
                    layer == 2 ? UITheme::Dark().success : UITheme::Dark().accent});
                if (layer > 0) for (std::uint64_t previous = 0; previous < 3; ++previous) {
                    edges.push_back({(layer - 1) * 3 + previous, layer * 3 + row,
                                    UITheme::Dark().textSecondary, true});
                }
            }
        }
        network->SetGraph(std::move(nodes), std::move(edges));
        network->SetSelectedNode(4);
        selection = &Text(networkPage, font, "Selected node: 4 / 9 nodes, 18 directed edges", 12);
        Text(tablePage, font, "Run comparison", 18);
        Text(tablePage, font, "Click a row / scroll / Up, Down, Home, End, Page Up and Page Down");
        auto &table = tablePage.CreateChild<TableView>(font);
        tablePage.SetChildFlex(table, 1);
        std::vector<TableRow> tableRows;
        for (int i = 0; i < 40; ++i) {
            tableRows.push_back({"run-" + std::to_string(i),
                {"Run " + std::to_string(i + 1), i % 3 == 0 ? "Baseline — extended observation" : "Candidate",
                 std::to_string(100 + i * 7), i % 4 == 0 ? "Pending" : "Complete"}});
        }
        table.SetData({{"Run", 1}, {"Model", 2}, {"Score", 1}, {"Status", 1}}, std::move(tableRows));
        table.SetSelectedRow("run-0");
        auto &tableSelection = Text(tablePage, font, "Selected: run-0 / 40 sample rows", 12);
        table.SetOnSelectionChanged([&tableSelection](std::optional<std::string> id) {
            tableSelection.SetText("Selected: " + id.value_or("none") + " / 40 sample rows");
        });

        using namespace pipeframe::ui;
        Compose(*plots,
            Component<Column>("history", [](Column &column) { Transparent(column); }),
            Component<Column>("scores", [](Column &column) { Transparent(column); }));
        auto &history = *static_cast<Column *>(plots->FindChildByKey("history"));
        auto &scores = *static_cast<Column *>(plots->FindChildByKey("scores"));
        plots->SetChildFlex(history, 1);
        plots->SetChildFlex(scores, 1);
        Text(history, font, "Time series", 18);
        Text(history, font, "Blue: signal / green: reference | time 0-60");
        line = &history.CreateChild<TimeSeriesChart>();
        history.SetChildFlex(*line, 1);
        line->SetVerticalRange(ChartRange{-1.5, 1.5});
        Text(history, font, "Range -1.5 to +1.5 / missing samples break the line", 12);
        Text(scores, font, "Category comparison", 18);
        Text(scores, font, "Signed values / categories A-F");
        bars = &scores.CreateChild<BarChart>();
        scores.SetChildFlex(*bars, 1);
        bars->SetVerticalRange(ChartRange{-1.5, 1.5});
        Text(scores, font, "Range -1.5 to +1.5 / bars start at zero", 12);

        auto &readouts = root->CreateChild<Row>();
        Transparent(readouts);
        readouts.SetSize({0, 96});
        metric = &readouts.CreateChild<MetricCard>(font);
        metric->SetTitle("SIGNAL GAIN");
        metric->SetDetail("Drag the slider to change both plots");
        readouts.SetChildFlex(*metric, 1);
        gauge = &readouts.CreateChild<Gauge>();
        gauge->SetSize({96, 96});
        progress = &root->CreateChild<ProgressBar>();
        progress->SetSize({0, 8});
        Text(*root, font, "Gain / 0.0-1.5");
        auto &slider = root->CreateChild<Slider>();
        slider.SetSize({0, 32});
        slider.SetRange(0, 1.5f);
        slider.SetStep(0.05f);
        slider.SetValue(1);
        slider.SetOnValueChanged([this](float value) { gain = value; Refresh(); });
        auto &animation = root->CreateChild<Row>();
        Transparent(animation);
        animation.SetSize({0, 32});
        auto &toggle = animation.CreateChild<Toggle>();
        toggle.SetSize({48, 28});
        toggle.SetChecked(true);
        toggle.SetOnChanged([this](bool value) { running = value; });
        auto &caption = Text(animation, font, "Animate signal / keyboard: Tab, Space, arrows");
        animation.SetChildFlex(caption, 1);
        Refresh();
    }

    void Resize(sf::Vector2u size) {
        root->SetSize(sf::Vector2f(size));
        plots->SetOrientation(size.x < 760 ? StackOrientation::Vertical : StackOrientation::Horizontal);
    }
    void Update(float delta) {
        ui.Update(delta);
        if (!running) return;
        elapsed += delta;
        if (elapsed >= 0.1f) {
            phase += elapsed;
            elapsed = 0;
            Refresh();
        }
    }
    UIManager ui;
    std::function<void()> onControls;
    void ShowNetwork() { tabs->SetSelectedIndex(1); }
    void ShowTable() { tabs->SetSelectedIndex(2); }
    void HandleEvent(const sf::Event &event) {
        if (const auto *press = event.getIf<sf::Event::MouseButtonPressed>();
            press && press->button == sf::Mouse::Button::Left && tabs->GetSelectedIndex() == 1) {
            if (const auto id = network->FindNodeAt(sf::Vector2f(press->position))) {
                network->SetSelectedNode(id);
                selection->SetText("Selected node: " + std::to_string(*id) + " / 9 nodes, 18 directed edges");
            }
        }
        ui.HandleEvent(event);
    }

  private:
    void Refresh() {
        ChartSeries signal, reference;
        reference.color = UITheme::Dark().success;
        for (int i = 0; i <= 60; ++i) {
            signal.samples.emplace_back(static_cast<float>(i), gain * std::sin(i * 0.14f + phase));
            reference.samples.emplace_back(static_cast<float>(i), 0.45f * std::cos(i * 0.1f));
        }
        line->SetSeries({std::move(signal), std::move(reference)});
        bars->SetValues({gain * 0.4f, gain * 0.9f, gain * -0.35f, gain * 0.65f, gain * -0.6f, gain});
        metric->SetValueText(std::to_string(gain).substr(0, 4) + "x");
        gauge->SetValue(gain / 1.5f, false);
        progress->SetValue(gain / 1.5f, false);
    }
    Column *root = nullptr;
    TabView *tabs = nullptr;
    NetworkView *network = nullptr;
    Label *selection = nullptr;
    StackPanel *plots = nullptr;
    TimeSeriesChart *line = nullptr;
    BarChart *bars = nullptr;
    MetricCard *metric = nullptr;
    Gauge *gauge = nullptr;
    ProgressBar *progress = nullptr;
    float gain = 1, phase = 0, elapsed = 0;
    bool running = true;
};
} // namespace

int main(int argc, char **argv) {
    sf::Font font;
    if (!font.openFromFile(PIPEFRAME_GALLERY_FONT)) {
        std::cerr << "Cannot load gallery font.\n";
        return 1;
    }
    Gallery gallery(font);
    ControlsGallery controls(font);
    bool showControls = argc >= 2 && std::string(argv[1]) == "--controls";
    gallery.onControls = [&] { showControls = true; };
    controls.SetOnMonitoring([&] { showControls = false; });
    if (argc >= 2 && std::string(argv[1]) == "--check") return controls.CheckInteractions() ? 0 : 1;
    // Deterministic screenshot mode also exercises resize layout without a window/event loop.
    if (argc >= 3 && std::string(argv[1]) == "--snapshot") {
        const sf::Vector2u size = argc >= 5
            ? sf::Vector2u{static_cast<unsigned int>(std::stoul(argv[3])), static_cast<unsigned int>(std::stoul(argv[4]))}
            : sf::Vector2u{1100, 800};
        const float scale = argc >= 7 ? std::stof(argv[6]) : 1.0f;
        if (!std::isfinite(scale) || scale <= 0 || size.x == 0 || size.y == 0) return 1;
        const sf::Vector2u logicalSize{static_cast<unsigned int>(size.x / scale),static_cast<unsigned int>(size.y / scale)};
        if (logicalSize.x == 0 || logicalSize.y == 0) return 1;
        gallery.Resize(logicalSize);
        controls.Resize(logicalSize);
        if (argc >= 6 && std::string(argv[5]) == "network") gallery.ShowNetwork();
        if (argc >= 6 && std::string(argv[5]) == "table") gallery.ShowTable();
        if (argc >= 6) {
            const std::string scene = argv[5];
            showControls = scene != "charts" && scene != "network" && scene != "table";
            if (showControls) controls.Present(scene);
        }
        gallery.ui.Update(0.25f);
        controls.Update(0.6f);
        controls.Update(0.3f);
        sf::RenderTexture target(size);
        target.setView(sf::View(sf::FloatRect{{0,0},sf::Vector2f(logicalSize)}));
        target.clear(UITheme::Dark().applicationBackground);
        if (showControls) controls.Render(target); else gallery.ui.Render(target);
        target.display();
        return target.getTexture().copyToImage().saveToFile(argv[2]) ? 0 : 1;
    }
    sf::RenderWindow window(sf::VideoMode({1100, 800}), "PipeFrame UI Gallery");
    window.setFramerateLimit(60);
    gallery.Resize(window.getSize());
    controls.Resize(window.getSize());
    sf::Clock clock;
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto *resize = event->getIf<sf::Event::Resized>()) {
                window.setView(sf::View(sf::FloatRect{{0, 0}, sf::Vector2f(resize->size)}));
                gallery.Resize(resize->size);
                controls.Resize(resize->size);
            }
            if (showControls) controls.HandleEvent(*event); else gallery.HandleEvent(*event);
        }
        if (!window.isOpen()) break;
        const float delta = clock.restart().asSeconds();
        if (showControls) controls.Update(delta); else gallery.Update(delta);
        window.clear(UITheme::Dark().applicationBackground);
        if (showControls) controls.Render(window); else gallery.ui.Render(window);
        window.display();
    }
}
