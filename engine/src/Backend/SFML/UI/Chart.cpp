#include <PipeFrame/Backend/SFML/UI/Chart.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
bool Finite(const sf::Vector2f point) { return std::isfinite(point.x) && std::isfinite(point.y); }

void Validate(const std::optional<ChartRange> &range) {
    if (range && (!std::isfinite(range->minimum) || !std::isfinite(range->maximum) ||
                  range->minimum >= range->maximum || !std::isfinite(range->maximum - range->minimum))) {
        throw std::invalid_argument("Chart ranges must have finite, increasing bounds and span");
    }
}

ChartRange Resolve(ChartRange range) {
    if (range.minimum > range.maximum)
        return {0.0, 1.0};
    if (range.minimum == range.maximum) {
        const double padding = std::max(0.5, std::abs(range.minimum) * 0.05);
        return {range.minimum - padding, range.maximum + padding};
    }
    return range;
}

sf::Color Fade(sf::Color color, const float opacity) {
    color.a = static_cast<std::uint8_t>(std::lround(color.a * std::clamp(opacity, 0.0f, 1.0f)));
    return color;
}

// Clip in data coordinates before normalization, including for very narrow fixed ranges.
bool Clip(double &x0, double &y0, double &x1, double &y1, ChartRange x, ChartRange y) {
    const double dx = x1 - x0, dy = y1 - y0;
    double enter = 0.0, leave = 1.0;
    const auto edge = [&](const double p, const double q) {
        if (p == 0.0)
            return q >= 0.0;
        const double ratio = q / p;
        if (p < 0.0)
            enter = std::max(enter, ratio);
        else
            leave = std::min(leave, ratio);
        return enter <= leave;
    };
    if (!edge(-dx, x0 - x.minimum) || !edge(dx, x.maximum - x0) || !edge(-dy, y0 - y.minimum) ||
        !edge(dy, y.maximum - y0))
        return false;
    x1 = x0 + leave * dx;
    y1 = y0 + leave * dy;
    x0 += enter * dx;
    y0 += enter * dy;
    return true;
}
} // namespace

Chart::Chart(const Kind newKind, const UITheme &theme) : kind(newKind), gridColor(theme.subtleBorder) {
    SetHitTestVisible(false);
    SetSize({240.0f, 120.0f});
}

void Chart::SetSeries(std::vector<ChartSeries> newSeries) {
    series = std::move(newSeries);
    RebuildGeometry();
}
const std::vector<ChartSeries> &Chart::GetSeries() const { return series; }
void Chart::SetHorizontalRange(const std::optional<ChartRange> range) {
    Validate(range);
    horizontalRange = range;
    RebuildGeometry();
}
void Chart::SetVerticalRange(const std::optional<ChartRange> range) {
    Validate(range);
    verticalRange = range;
    RebuildGeometry();
}
ChartRange Chart::GetHorizontalRange() const { return resolvedHorizontal; }
ChartRange Chart::GetVerticalRange() const { return resolvedVertical; }
void Chart::SetGridColor(const sf::Color color) {
    gridColor = color;
    RebuildGeometry();
}
void Chart::SetGridDivisions(const unsigned int divisions) {
    gridDivisions = std::min(divisions, 32u);
    RebuildGeometry();
}
void Chart::OnGeometryChanged() { RebuildGeometry(); }
void Chart::OnOpacityChanged() { RebuildGeometry(); }
void Chart::OnRender(sf::RenderTarget &target) const {
    if (!gridVertices.empty())
        target.draw(gridVertices.data(), gridVertices.size(), sf::PrimitiveType::Lines);
    if (!plotVertices.empty())
        target.draw(plotVertices.data(), plotVertices.size(),
                    kind == Kind::Line ? sf::PrimitiveType::Lines : sf::PrimitiveType::Triangles);
}

void Chart::RebuildGeometry() {
    const double infinity = std::numeric_limits<double>::infinity();
    ChartRange x{infinity, -infinity}, y{infinity, -infinity};
    for (const auto &entry : series)
        for (const auto point : entry.samples) {
            if (!Finite(point))
                continue;
            const double halfWidth = kind == Kind::Bar ? 0.5 : 0.0;
            x.minimum = std::min(x.minimum, point.x - halfWidth);
            x.maximum = std::max(x.maximum, point.x + halfWidth);
            y.minimum = std::min(y.minimum, static_cast<double>(point.y));
            y.maximum = std::max(y.maximum, static_cast<double>(point.y));
        }
    if (kind == Kind::Bar && y.minimum <= y.maximum) {
        y.minimum = std::min(y.minimum, 0.0);
        y.maximum = std::max(y.maximum, 0.0);
    }
    resolvedHorizontal = horizontalRange.value_or(Resolve(x));
    resolvedVertical = verticalRange.value_or(Resolve(y));
    gridVertices.clear();
    plotVertices.clear();
    const auto size = GetSize(), origin = GetScreenPosition();
    if (size.x <= 0.0f || size.y <= 0.0f)
        return;
    const auto screen = [&](const double px, const double py) {
        return origin + sf::Vector2f{static_cast<float>(std::clamp(px, 0.0, 1.0) * size.x),
                                     static_cast<float>((1.0 - std::clamp(py, 0.0, 1.0)) * size.y)};
    };
    const auto nx = [&](const double value) {
        return (value - resolvedHorizontal.minimum) / (resolvedHorizontal.maximum - resolvedHorizontal.minimum);
    };
    const auto ny = [&](const double value) {
        return (value - resolvedVertical.minimum) / (resolvedVertical.maximum - resolvedVertical.minimum);
    };
    const auto grid = Fade(gridColor, GetEffectiveOpacity());
    if (gridDivisions > 0)
        for (unsigned int i = 0; i <= gridDivisions; ++i) {
            const double fraction = static_cast<double>(i) / gridDivisions;
            gridVertices.emplace_back(screen(0.0, fraction), grid);
            gridVertices.emplace_back(screen(1.0, fraction), grid);
            gridVertices.emplace_back(screen(fraction, 0.0), grid);
            gridVertices.emplace_back(screen(fraction, 1.0), grid);
        }
    for (const auto &entry : series) {
        const auto color = Fade(entry.color, GetEffectiveOpacity());
        for (std::size_t i = 0; i < entry.samples.size(); ++i) {
            const auto point = entry.samples[i];
            if (!Finite(point))
                continue;
            if (kind == Kind::Line) {
                if (i == 0 || !Finite(entry.samples[i - 1]))
                    continue;
                double x0 = entry.samples[i - 1].x, y0 = entry.samples[i - 1].y;
                double x1 = point.x, y1 = point.y;
                if (Clip(x0, y0, x1, y1, resolvedHorizontal, resolvedVertical)) {
                    plotVertices.emplace_back(screen(nx(x0), ny(y0)), color);
                    plotVertices.emplace_back(screen(nx(x1), ny(y1)), color);
                }
            } else {
                const double left = std::max(0.0, nx(static_cast<double>(point.x) - 0.4));
                const double right = std::min(1.0, nx(static_cast<double>(point.x) + 0.4));
                const double bottom = std::max(0.0, ny(std::min(0.0f, point.y)));
                const double top = std::min(1.0, ny(std::max(0.0f, point.y)));
                if (left >= right || bottom >= top)
                    continue;
                for (const auto vertex : {screen(left, bottom), screen(right, bottom), screen(left, top),
                                          screen(left, top), screen(right, bottom), screen(right, top)}) {
                    plotVertices.emplace_back(vertex, color);
                }
            }
        }
    }
}

void BarChart::SetValues(const std::vector<float> &values, const sf::Color color) {
    ChartSeries entry;
    entry.color = color;
    entry.samples.reserve(values.size());
    for (std::size_t i = 0; i < values.size(); ++i)
        entry.samples.emplace_back(static_cast<float>(i), values[i]);
    SetSeries({std::move(entry)});
}
