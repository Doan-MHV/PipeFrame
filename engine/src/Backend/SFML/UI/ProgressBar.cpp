#include <PipeFrame/Backend/SFML/UI/ProgressBar.h>

#include <algorithm>

ProgressBar::ProgressBar(const UITheme &theme)
    : fill(CreateChild<Panel>()), transitionDuration(theme.motionNormal) {
    SetSize({160.0f, 8.0f});
    SetFillColor(theme.controlNormal);
    SetOutlineColor(sf::Color::Transparent);
    SetOutlineThickness(0.0f);
    SetCornerRadius(theme.radiusPill);
    SetHitTestVisible(false);

    fill.SetFillColor(theme.accent);
    fill.SetOutlineColor(sf::Color::Transparent);
    fill.SetOutlineThickness(0.0f);
    fill.SetCornerRadius(theme.radiusPill);
    fill.SetHitTestVisible(false);
    RefreshFill();
}

void ProgressBar::SetValue(const float newValue, const bool animate) {
    value = std::clamp(newValue, 0.0f, 1.0f);
    animatedValue.SetTarget(value, !animate || IsReducedMotion());
    RefreshFill();
}

float ProgressBar::GetValue() const { return value; }
float ProgressBar::GetVisualValue() const { return animatedValue.Get(); }
void ProgressBar::SetTrackColor(const sf::Color color) { Panel::SetFillColor(color); }
void ProgressBar::SetFillColorRole(const sf::Color color) { fill.SetFillColor(color); }

void ProgressBar::SetTransitionDuration(const float seconds) {
    transitionDuration = std::max(0.0f, seconds);
}

void ProgressBar::SetReducedMotion(const bool reducedMotion) {
    Widget::SetReducedMotion(reducedMotion);
    if (reducedMotion) {
        animatedValue.SetTarget(value, true);
        RefreshFill();
    }
}

void ProgressBar::OnGeometryChanged() {
    Panel::OnGeometryChanged();
    RefreshFill();
}

void ProgressBar::OnUpdate(const float realDeltaSeconds) {
    if (animatedValue.Update(realDeltaSeconds,
                             IsReducedMotion() ? 0.0f : transitionDuration)) {
        RefreshFill();
    }
}

void ProgressBar::RefreshFill() {
    fill.Arrange({{0.0f, 0.0f}, {GetSize().x * animatedValue.Get(), GetSize().y}});
}
