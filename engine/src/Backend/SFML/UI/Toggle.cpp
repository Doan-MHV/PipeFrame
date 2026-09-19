#include <PipeFrame/Backend/SFML/UI/Toggle.h>

#include <algorithm>
#include <utility>

Toggle::Toggle(const UITheme &newTheme) : knob(CreateChild<Panel>()), theme(newTheme) {
    SetSize({44.0f, 24.0f});
    SetCornerRadius(theme.radiusPill);
    SetNormalColor(theme.controlNormal);
    SetHoveredColor(theme.controlHovered);
    SetSelectedColor(theme.accent);
    knob.SetFillColor(theme.textPrimary);
    knob.SetOutlineColor(sf::Color::Transparent);
    knob.SetOutlineThickness(0.0f);
    knob.SetCornerRadius(theme.radiusPill);
    knob.SetHitTestVisible(false);
    SetOnClick([this]() { SetChecked(!checked, true); });
    RefreshKnob(false);
}

void Toggle::SetChecked(const bool newChecked, const bool notify) {
    if (checked == newChecked) {
        return;
    }
    checked = newChecked;
    SetSelected(checked);
    RefreshKnob(true);
    if (notify && onChanged) {
        onChanged(checked);
    }
}

bool Toggle::IsChecked() const { return checked; }

void Toggle::SetOnChanged(ChangedCallback callback) { onChanged = std::move(callback); }

void Toggle::OnGeometryChanged() {
    Button::OnGeometryChanged();
    const float diameter = std::max(1.0f, GetSize().y - 8.0f);
    knob.SetPosition({4.0f, 4.0f});
    knob.SetSize({diameter, diameter});
    knob.SetCornerRadius(diameter * 0.5f);
    if (!knob.IsMotionActive()) {
        RefreshKnob(false);
    }
}

void Toggle::RefreshKnob(const bool animate) {
    const float diameter = std::max(1.0f, GetSize().y - 8.0f);
    const float travel = std::max(0.0f, GetSize().x - diameter - 8.0f);
    const sf::Vector2f offset{checked ? travel : 0.0f, 0.0f};
    if (animate && !IsReducedMotion()) {
        knob.AnimateVisualOffsetTo(offset, theme.motionFast);
    } else {
        knob.SetVisualOffset(offset);
    }
}
