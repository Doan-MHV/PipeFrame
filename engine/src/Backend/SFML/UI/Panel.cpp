#include <PipeFrame/Backend/SFML/UI/Panel.h>

#include <algorithm>
#include <cmath>

namespace {
sf::Color ApplyOpacity(sf::Color color, const float opacity) {
    color.a = static_cast<std::uint8_t>(std::clamp(
        std::lround(static_cast<float>(color.a) * std::clamp(opacity, 0.0f, 1.0f)), 0l, 255l));
    return color;
}
} // namespace

Panel::Panel() {
    RefreshOpacity();
    background.setOutlineThickness(1.0f);

    OnGeometryChanged();
}

void Panel::SetFillColor(const sf::Color color) {
    fillColor = color;
    RefreshOpacity();
}

void Panel::SetOutlineColor(const sf::Color color) {
    outlineColor = color;
    RefreshOpacity();
}

void Panel::SetOutlineThickness(float thickness) { background.setOutlineThickness(thickness); }

void Panel::SetCornerRadius(const float radius) {
    background.setCornerRadius(radius);
    shadow.setCornerRadius(radius);
}

void Panel::SetShadowColor(const sf::Color color) {
    shadowColor = color;
    RefreshOpacity();
}

void Panel::SetShadowOffset(const sf::Vector2f offset) {
    shadowOffset = offset;
    OnGeometryChanged();
}

sf::Color Panel::GetFillColor() const { return fillColor; }

sf::Color Panel::GetOutlineColor() const { return outlineColor; }

float Panel::GetOutlineThickness() const { return background.getOutlineThickness(); }

float Panel::GetCornerRadius() const { return background.getCornerRadius(); }

sf::Color Panel::GetShadowColor() const { return shadowColor; }

sf::Vector2f Panel::GetShadowOffset() const { return shadowOffset; }

void Panel::OnRender(sf::RenderTarget &target) const {
    if (shadow.getFillColor().a > 0) {
        target.draw(shadow);
    }
    target.draw(background);
}

void Panel::OnGeometryChanged() {
    background.setPosition(GetScreenPosition());
    background.setSize(GetSize());
    shadow.setPosition(GetScreenPosition() + shadowOffset);
    shadow.setSize(GetSize());
}

void Panel::OnOpacityChanged() { RefreshOpacity(); }

void Panel::RefreshOpacity() {
    const float opacity = GetEffectiveOpacity();
    background.setFillColor(ApplyOpacity(fillColor, opacity));
    background.setOutlineColor(ApplyOpacity(outlineColor, opacity));
    shadow.setFillColor(ApplyOpacity(shadowColor, opacity));
}
