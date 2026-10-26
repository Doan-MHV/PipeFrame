#include <PipeFrame/Backend/SFML/UI/Gauge.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

sf::Color ApplyOpacity(sf::Color color, const float opacity) {
    color.a = static_cast<std::uint8_t>(
        std::clamp(std::lround(static_cast<float>(color.a) * std::clamp(opacity, 0.0f, 1.0f)), 0l, 255l));
    return color;
}

void AppendArc(std::vector<sf::Vertex> &vertices, const sf::Vector2f center, const float innerRadius,
               const float outerRadius, const float startDegrees, const float sweepDegrees, const sf::Color color) {
    const int segments = std::max(1, static_cast<int>(std::ceil(std::abs(sweepDegrees) / 6.0f)));
    vertices.clear();
    vertices.reserve(static_cast<std::size_t>(segments) * 6);
    for (int segment = 0; segment < segments; ++segment) {
        const float first = static_cast<float>(segment) / static_cast<float>(segments);
        const float second = static_cast<float>(segment + 1) / static_cast<float>(segments);
        const float angle0 = (startDegrees + sweepDegrees * first) * std::numbers::pi_v<float> / 180.0f;
        const float angle1 = (startDegrees + sweepDegrees * second) * std::numbers::pi_v<float> / 180.0f;
        const sf::Vector2f direction0{std::cos(angle0), std::sin(angle0)};
        const sf::Vector2f direction1{std::cos(angle1), std::sin(angle1)};
        const sf::Vector2f outer0 = center + direction0 * outerRadius;
        const sf::Vector2f inner0 = center + direction0 * innerRadius;
        const sf::Vector2f outer1 = center + direction1 * outerRadius;
        const sf::Vector2f inner1 = center + direction1 * innerRadius;
        vertices.emplace_back(outer0, color);
        vertices.emplace_back(inner0, color);
        vertices.emplace_back(outer1, color);
        vertices.emplace_back(outer1, color);
        vertices.emplace_back(inner0, color);
        vertices.emplace_back(inner1, color);
    }
}

} // namespace

Gauge::Gauge(const UITheme &theme)
    : trackColor(theme.controlNormal), fillColor(theme.accent), transitionDuration(theme.motionNormal) {
    SetSize({72.0f, 72.0f});
    SetHitTestVisible(false);
    RebuildGeometry();
}

void Gauge::SetRange(float newMinimum, float newMaximum) {
    if (newMinimum > newMaximum) {
        std::swap(newMinimum, newMaximum);
    }
    minimum = newMinimum;
    maximum = newMaximum;
    SetValue(value, false);
}

void Gauge::SetValue(const float newValue, const bool animate) {
    value = std::clamp(newValue, minimum, maximum);
    animatedValue.SetTarget(value, !animate || IsReducedMotion());
    RebuildGeometry();
}

float Gauge::GetValue() const { return value; }
float Gauge::GetVisualValue() const { return animatedValue.Get(); }
void Gauge::SetTrackColor(const sf::Color color) {
    trackColor = color;
    RebuildGeometry();
}
void Gauge::SetFillColor(const sf::Color color) {
    fillColor = color;
    RebuildGeometry();
}
void Gauge::SetThickness(const float newThickness) {
    thickness = std::max(1.0f, newThickness);
    RebuildGeometry();
}
void Gauge::SetSweep(const float newStartDegrees, const float newSweepDegrees) {
    startDegrees = newStartDegrees;
    sweepDegrees = std::clamp(newSweepDegrees, -360.0f, 360.0f);
    RebuildGeometry();
}
void Gauge::SetTransitionDuration(const float seconds) { transitionDuration = std::max(0.0f, seconds); }
void Gauge::SetReducedMotion(const bool reducedMotion) {
    Widget::SetReducedMotion(reducedMotion);
    if (reducedMotion) {
        animatedValue.SetTarget(value, true);
        RebuildGeometry();
    }
}

void Gauge::OnRender(sf::RenderTarget &target) const {
    target.draw(trackVertices.data(), trackVertices.size(), sf::PrimitiveType::Triangles);
    target.draw(fillVertices.data(), fillVertices.size(), sf::PrimitiveType::Triangles);
}
void Gauge::OnGeometryChanged() { RebuildGeometry(); }
void Gauge::OnOpacityChanged() { RebuildGeometry(); }
void Gauge::OnUpdate(const float realDeltaSeconds) {
    if (animatedValue.Update(realDeltaSeconds, IsReducedMotion() ? 0.0f : transitionDuration)) {
        RebuildGeometry();
    }
}

float Gauge::NormalizedVisualValue() const {
    const float range = maximum - minimum;
    return range <= 0.0f ? 0.0f : std::clamp((animatedValue.Get() - minimum) / range, 0.0f, 1.0f);
}

void Gauge::RebuildGeometry() {
    const sf::Vector2f size = GetSize();
    const sf::Vector2f center = GetScreenPosition() + size * 0.5f;
    const float outerRadius = std::max(0.0f, std::min(size.x, size.y) * 0.5f);
    const float innerRadius = std::max(0.0f, outerRadius - thickness);
    AppendArc(trackVertices, center, innerRadius, outerRadius, startDegrees, sweepDegrees,
              ApplyOpacity(trackColor, GetEffectiveOpacity()));
    AppendArc(fillVertices, center, innerRadius, outerRadius, startDegrees, sweepDegrees * NormalizedVisualValue(),
              ApplyOpacity(fillColor, GetEffectiveOpacity()));
}
