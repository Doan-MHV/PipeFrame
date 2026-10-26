#include <PipeFrame/Backend/SFML/UI/Slider.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

Slider::Slider(const UITheme &newTheme)
    : track(CreateChild<Panel>()), filledTrack(CreateChild<Panel>()), thumb(CreateChild<Panel>()), theme(newTheme) {
    SetSize({160.0f, theme.controlHeight});
    SetFillColor(sf::Color::Transparent);
    SetOutlineColor(sf::Color::Transparent);
    SetOutlineThickness(0.0f);
    SetFocusable(true);

    track.SetFillColor(theme.controlNormal);
    track.SetOutlineColor(sf::Color::Transparent);
    track.SetOutlineThickness(0.0f);
    track.SetCornerRadius(theme.radiusPill);
    track.SetHitTestVisible(false);

    filledTrack.SetFillColor(theme.accent);
    filledTrack.SetOutlineColor(sf::Color::Transparent);
    filledTrack.SetOutlineThickness(0.0f);
    filledTrack.SetCornerRadius(theme.radiusPill);
    filledTrack.SetHitTestVisible(false);

    thumb.SetFillColor(theme.textPrimary);
    thumb.SetOutlineColor(theme.border);
    thumb.SetOutlineThickness(1.0f);
    thumb.SetCornerRadius(theme.radiusPill);
    thumb.SetHitTestVisible(false);
    RefreshGeometry();
}

void Slider::SetRange(float newMinimum, float newMaximum) {
    if (newMinimum > newMaximum) {
        std::swap(newMinimum, newMaximum);
    }
    minimum = newMinimum;
    maximum = newMaximum;
    value = SanitizeValue(value);
    RefreshGeometry();
}

float Slider::GetMinimum() const { return minimum; }

float Slider::GetMaximum() const { return maximum; }

void Slider::SetStep(const float newStep) {
    step = std::max(0.0f, newStep);
    value = SanitizeValue(value);
    RefreshGeometry();
}

float Slider::GetStep() const { return step; }

void Slider::SetValue(const float newValue) {
    const float sanitized = SanitizeValue(newValue);
    if (value == sanitized) {
        return;
    }
    value = sanitized;
    RefreshGeometry();
}

float Slider::GetValue() const { return value; }

float Slider::GetNormalizedValue() const {
    const float range = maximum - minimum;
    return range <= 0.0f ? 0.0f : (value - minimum) / range;
}

void Slider::SetOnValueChanged(ValueChangedCallback callback) { onValueChanged = std::move(callback); }

bool Slider::OnEvent(const sf::Event &event) {
    if (const auto *pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (pressed->button != sf::Mouse::Button::Left) {
            return false;
        }
        dragging = true;
        SetValueFromPointer(static_cast<float>(pressed->position.x));
        return true;
    }
    if (const auto *moved = event.getIf<sf::Event::MouseMoved>()) {
        if (!dragging) {
            return false;
        }
        SetValueFromPointer(static_cast<float>(moved->position.x));
        return true;
    }
    if (const auto *released = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (released->button != sf::Mouse::Button::Left || !dragging) {
            return false;
        }
        dragging = false;
        SetValueFromPointer(static_cast<float>(released->position.x));
        return true;
    }
    if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
        const float keyboardStep = step > 0.0f ? step : std::max((maximum - minimum) * 0.01f, 0.01f);
        if (key->code == sf::Keyboard::Key::Left) {
            SetValueFromInput(value - keyboardStep);
            return true;
        }
        if (key->code == sf::Keyboard::Key::Right) {
            SetValueFromInput(value + keyboardStep);
            return true;
        }
        if (key->code == sf::Keyboard::Key::Home) {
            SetValueFromInput(minimum);
            return true;
        }
        if (key->code == sf::Keyboard::Key::End) {
            SetValueFromInput(maximum);
            return true;
        }
    }
    return false;
}

void Slider::OnGeometryChanged() {
    Panel::OnGeometryChanged();
    RefreshGeometry();
}

void Slider::OnEnabledChanged() {
    dragging = false;
    RefreshVisual();
}

void Slider::OnKeyboardFocusGained() { RefreshVisual(); }

void Slider::OnKeyboardFocusLost() {
    dragging = false;
    RefreshVisual();
}

void Slider::SetValueFromInput(const float newValue) {
    const float previous = value;
    SetValue(newValue);
    if (value != previous && onValueChanged) {
        onValueChanged(value);
    }
}

void Slider::SetValueFromPointer(const float screenX) {
    const float width = std::max(1.0f, GetSize().x);
    const float normalized = std::clamp((screenX - GetScreenPosition().x) / width, 0.0f, 1.0f);
    SetValueFromInput(minimum + (maximum - minimum) * normalized);
}

float Slider::SanitizeValue(float candidate) const {
    candidate = std::clamp(candidate, minimum, maximum);
    if (step > 0.0f) {
        candidate = minimum + std::round((candidate - minimum) / step) * step;
    }
    return std::clamp(candidate, minimum, maximum);
}

void Slider::RefreshGeometry() {
    const sf::Vector2f size = GetSize();
    const float trackHeight = 4.0f;
    const float thumbSize = std::min(16.0f, std::max(8.0f, size.y - 8.0f));
    const float centerY = size.y * 0.5f;
    const float progress = GetNormalizedValue();

    track.SetPosition({0.0f, centerY - trackHeight * 0.5f});
    track.SetSize({size.x, trackHeight});
    filledTrack.SetPosition({0.0f, centerY - trackHeight * 0.5f});
    filledTrack.SetSize({size.x * progress, trackHeight});
    thumb.SetPosition({std::clamp(size.x * progress - thumbSize * 0.5f, 0.0f, std::max(0.0f, size.x - thumbSize)),
                       centerY - thumbSize * 0.5f});
    thumb.SetSize({thumbSize, thumbSize});
    thumb.SetCornerRadius(thumbSize * 0.5f);
}

void Slider::RefreshVisual() {
    filledTrack.SetFillColor(IsEnabled() ? theme.accent : theme.controlDisabled);
    thumb.SetFillColor(IsEnabled() ? theme.textPrimary : theme.textDisabled);
    thumb.SetOutlineColor(HasKeyboardFocus() ? theme.accentHovered : theme.border);
    thumb.SetOutlineThickness(HasKeyboardFocus() ? 2.0f : 1.0f);
}
