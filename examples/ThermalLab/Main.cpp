#include "ThermalLab.h"
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <filesystem>
#include <iostream>

bool Check(bool okay, const char *message) {
    if (!okay)
        std::cerr << "FAILED: " << message << '\n';
    return okay;
}
sf::Image Snapshot(ThermalLab &lab, sf::Vector2u pixels, float scale) {
    const sf::Vector2f logical = sf::Vector2f(pixels) / scale;
    lab.Layout(logical);
    lab.Update(0.3f); // Flush and settle mounted UI while the fixture simulation remains paused.
    sf::RenderTexture target(pixels);
    target.setView(sf::View(sf::FloatRect{{}, logical}));
    target.clear(sf::Color::Black);
    lab.Render(target);
    target.display();
    return target.getTexture().copyToImage();
}
bool Compare(const sf::Image &actual, const sf::Image &golden) {
    if (actual.getSize() != golden.getSize())
        return false;
    std::size_t different = 0;
    double error = 0;
    for (unsigned y = 0; y < actual.getSize().y; ++y)
        for (unsigned x = 0; x < actual.getSize().x; ++x) {
            const auto a = actual.getPixel({x, y}), b = golden.getPixel({x, y});
            const auto delta = std::max({std::abs(int(a.r) - int(b.r)), std::abs(int(a.g) - int(b.g)),
                                         std::abs(int(a.b) - int(b.b)), std::abs(int(a.a) - int(b.a))});
            different += delta > 8;
            error += delta;
        }
    const double count = actual.getSize().x * actual.getSize().y;
    std::cout << "Golden: changed=" << different / count << " mean channel error=" << error / count << '\n';
    return different / count < 0.005 && error / count < 0.5;
}
int main(int argc, char **argv) {
    sf::Font font;
    if (!font.openFromFile(PIPEFRAME_THERMAL_FONT))
        return 1;
    const bool update = argc > 1 && std::string(argv[1]) == "--update-goldens";
    if (update || (argc > 1 && std::string(argv[1]) == "--check")) {
        bool passed = true;
        struct Case {
            const char *file;
            sf::Vector2u pixels;
            float scale;
        };
        for (const auto &test : std::array{Case{"narrow.png", {640, 800}, 1}, Case{"wide.png", {1200, 800}, 1},
                                           Case{"scaled.png", {1200, 1200}, 1.5f}}) {
            ThermalLab lab(font);
            const auto actual = Snapshot(lab, test.pixels, test.scale);
            passed &= Check(lab.TargetSlider().GetSize().x > 100 && lab.Transport().GetSize().y >= 128,
                            "Responsive controls must keep usable dimensions");
            const auto path = std::filesystem::path(PIPEFRAME_THERMAL_GOLDENS) / test.file;
            if (update)
                passed &= actual.saveToFile(path);
            else {
                sf::Image golden;
                passed &= Check(golden.loadFromFile(path), "Golden must exist");
                if (golden.getSize().x)
                    passed &= Check(Compare(actual, golden), "Visual layout diverged from reviewed golden");
            }
        }
        ThermalLab lab(font);
        lab.Layout({640, 800});
        auto point = sf::Vector2i(lab.TargetSlider().GetScreenPosition() + lab.TargetSlider().GetSize() * 0.5f);
        passed &= Check(lab.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, point}),
                        "Slider consumes pointer input");
        lab.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, point});
        passed &= Check(lab.GetTarget() > 40 && lab.GetTarget() < 60, "Control writes through its state binding");
        lab.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Right});
        passed &= Check(lab.GetTarget() > 50, "Focused slider supports keyboard adjustment");
        const auto before = lab.GetTicks();
        lab.Update(0.1f);
        passed &= Check(lab.GetTicks() == before, "Paused simulation must not advance with real UI time");
        if (passed)
            std::cout << "ThermalLab acceptance passed.\n";
        return passed ? 0 : 1;
    }
    sf::RenderWindow window(sf::VideoMode({1200, 800}), "PipeFrame / Thermal lab");
    window.setFramerateLimit(60);
    ThermalLab lab(font);
    lab.Layout(sf::Vector2f(window.getSize()));
    sf::Clock clock;
    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (auto *resize = event->getIf<sf::Event::Resized>()) {
                window.setView(sf::View(sf::FloatRect{{}, sf::Vector2f(resize->size)}));
                lab.Layout(sf::Vector2f(resize->size));
            }
            lab.HandleEvent(*event);
        }
        lab.Update(std::min(0.1f, clock.restart().asSeconds()));
        window.clear();
        lab.Render(window);
        window.display();
    }
}
