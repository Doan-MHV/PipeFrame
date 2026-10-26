#include <PipeFrame/Backend/SFML/UI/Chart.h>
#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
bool Check(bool condition, const char *message) {
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}

sf::Image Render(const Widget &widget) {
    sf::RenderTexture target({160, 140});
    target.clear(sf::Color::Black);
    widget.Render(target);
    target.display();
    return target.getTexture().copyToImage();
}
bool LitNear(const sf::Image &image, unsigned int x, unsigned int y) {
    for (unsigned int row = y - 1; row <= y + 1; ++row)
        for (unsigned int column = x - 1; column <= x + 1; ++column)
            if (image.getPixel({column, row}).r > 100) return true;
    return false;
}
} // namespace

int main() {
    bool passed = true;
    TimeSeriesChart chart;
    chart.SetPosition({20.0f, 20.0f});
    chart.SetSize({100.0f, 100.0f});
    chart.SetGridDivisions(0);
    passed &= Check(chart.GetVerticalRange().minimum == 0.0 && chart.GetVerticalRange().maximum == 1.0,
                    "Empty charts must use a usable default range");
    chart.SetSeries({{{{2.0f, 7.0f}}, sf::Color::White}});
    passed &= Check(chart.GetVerticalRange().minimum < 7.0 && chart.GetVerticalRange().maximum > 7.0,
                    "Constant data must have a nonzero axis span");
    const auto oldRange = chart.GetVerticalRange();
    bool rejected = false;
    try { chart.SetVerticalRange(ChartRange{2.0, 2.0}); }
    catch (const std::invalid_argument &) { rejected = true; }
    passed &= Check(rejected && chart.GetVerticalRange().minimum == oldRange.minimum,
                    "Invalid axis updates must preserve the prior state");

    chart.SetHorizontalRange(ChartRange{0.0, 10.0});
    chart.SetVerticalRange(ChartRange{0.0, 10.0});
    chart.SetSeries({{{{-10.0f, -5.0f}, {20.0f, 10.0f}}, sf::Color::White}});
    auto image = Render(chart);
    passed &= Check(LitNear(image, 70, 95), "Clipped lines must preserve their original slope");
    passed &= Check(!LitNear(image, 70, 70), "Clipping must not clamp endpoints independently");
    passed &= Check(image.getPixel({10, 100}) == sf::Color::Black,
                    "Plots must not paint beyond their bounds");

    const float nan = std::numeric_limits<float>::quiet_NaN();
    chart.SetSeries({{{{0, 5}, {nan, nan}, {10, 5}}, sf::Color::White}});
    image = Render(chart);
    passed &= Check(!LitNear(image, 70, 70), "Missing samples must break the line");
    chart.SetHorizontalRange(std::nullopt);
    chart.SetVerticalRange(std::nullopt);
    chart.SetSeries({{{{nan, 4}, {1, std::numeric_limits<float>::infinity()}}, sf::Color::White}});
    passed &= Check(chart.GetVerticalRange().maximum == 1.0,
                    "Non-finite samples must not poison automatic ranges");
    const float largest = std::numeric_limits<float>::max();
    chart.SetSeries({{{{-largest, -largest}, {largest, largest}}, sf::Color::White}});
    passed &= Check(std::isfinite(chart.GetVerticalRange().maximum - chart.GetVerticalRange().minimum),
                    "Extreme float samples must retain finite axis spans");
    image = Render(chart);
    passed &= Check(LitNear(image, 70, 70), "Extreme finite samples must still render");

    Panel parent;
    parent.SetPosition({20, 20});
    parent.SetSize({100, 100});
    parent.SetOpacity(0.5f);
    auto &bars = parent.CreateChild<BarChart>();
    bars.SetSize({100, 100});
    bars.SetGridDivisions(0);
    bars.SetValues({-2, 2}, sf::Color::White);
    passed &= Check(bars.GetHorizontalRange().minimum == -0.5 && bars.GetHorizontalRange().maximum == 1.5,
                    "Automatic bar bounds must include the full category width");
    image = Render(bars);
    const auto faded = image.getPixel({45, 95}).r;
    passed &= Check(faded >= 126 && faded <= 129, "Bars must inherit parent opacity and position");
    passed &= Check(image.getPixel({45, 45}) == sf::Color::Black && image.getPixel({95, 45}).r > 100,
                    "Positive and negative bars must extend from zero in opposite directions");
    bars.SetValues({2, 4});
    passed &= Check(bars.GetVerticalRange().minimum == 0.0, "Positive bars must include zero automatically");
    bars.SetVerticalRange(ChartRange{1, 3});
    image = Render(bars);
    passed &= Check(image.getPixel({95, 25}).b > 100, "Fixed bar ranges must clip fills to the plot");
    bars.SetSize({0, 0});
    image = Render(bars);
    passed &= Check(image.getPixel({95, 25}) == sf::Color::Black, "Collapsed charts must clear cached geometry");

    UIManager manager;
    auto &passive = manager.CreateRoot<TimeSeriesChart>();
    passive.SetSize({100, 100});
    passed &= Check(!manager.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {30, 30}}),
                    "Passive charts must leave world input available");
    if (!passed) return 1;
    std::cout << "All chart tests passed.\n";
}
