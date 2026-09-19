#ifndef PIPEFRAME_UI_CHART_H
#define PIPEFRAME_UI_CHART_H

#include <PipeFrame/Backend/SFML/UI/UITheme.h>
#include <PipeFrame/Backend/SFML/UI/Widget.h>

#include <SFML/Graphics/Vertex.hpp>
#include <optional>
#include <vector>

struct ChartSeries {
    // Line samples are connected in supplied order; non-finite samples break the line.
    std::vector<sf::Vector2f> samples;
    sf::Color color = UITheme::Dark().accent;
};

struct ChartRange {
    double minimum = 0.0;
    double maximum = 1.0;
};

// Passive plotting surface. Compose labels, legends, and inspection controls around it.
// Data/history ownership and sampling policy remain with the application.
class Chart : public Widget {
public:
    void SetSeries(std::vector<ChartSeries> series);
    const std::vector<ChartSeries>& GetSeries() const;
    // nullopt restores automatic bounds. Invalid/degenerate fixed ranges throw.
    void SetHorizontalRange(std::optional<ChartRange> range);
    void SetVerticalRange(std::optional<ChartRange> range);
    ChartRange GetHorizontalRange() const;
    ChartRange GetVerticalRange() const;
    void SetGridColor(sf::Color color);
    void SetGridDivisions(unsigned int divisions);

protected:
    enum class Kind { Line, Bar };
    explicit Chart(Kind kind, const UITheme& theme);
    void OnRender(sf::RenderTarget& target) const override;
    void OnGeometryChanged() override;
    void OnOpacityChanged() override;

private:
    void RebuildGeometry();
    Kind kind;
    std::vector<ChartSeries> series;
    std::optional<ChartRange> horizontalRange;
    std::optional<ChartRange> verticalRange;
    ChartRange resolvedHorizontal;
    ChartRange resolvedVertical;
    sf::Color gridColor;
    unsigned int gridDivisions = 4;
    std::vector<sf::Vertex> gridVertices;
    std::vector<sf::Vertex> plotVertices;
};

class TimeSeriesChart final : public Chart {
public:
    explicit TimeSeriesChart(const UITheme& theme = UITheme::Dark()) : Chart(Kind::Line, theme) {}
};

class BarChart final : public Chart {
public:
    explicit BarChart(const UITheme& theme = UITheme::Dark()) : Chart(Kind::Bar, theme) {}
    // Categories occupy integer x positions; each bar is 0.8 data units wide.
    void SetValues(const std::vector<float>& values, sf::Color color = UITheme::Dark().accent);
};

#endif
