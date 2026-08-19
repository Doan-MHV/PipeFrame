#include <PipeFrame/UI/Button.h>

#include <utility>

#include <SFML/Window/Mouse.hpp>

Button::Button() {
    SetOutlineColor(sf::Color(76, 84, 102));
    SetOutlineThickness(1.0f);

    RefreshVisual();
}

void Button::SetOnClick(ClickCallback callback) { onClick = std::move(callback); }

void Button::SetNormalColor(sf::Color color) {
    normalColor = color;
    RefreshVisual();
}

void Button::SetHoveredColor(sf::Color color) {
    hoveredColor = color;
    RefreshVisual();
}

void Button::SetPressedColor(sf::Color color) {
    pressedColor = color;
    RefreshVisual();
}

void Button::SetDisabledColor(sf::Color color) {
    disabledColor = color;
    RefreshVisual();
}

ButtonState Button::GetState() const { return state; }

void Button::OnPointerEntered() {
    pointerInside = true;
    RefreshVisual();
}

void Button::OnPointerExited() {
    pointerInside = false;
    RefreshVisual();
}

void Button::OnEnabledChanged() {
    pressed = false;
    RefreshVisual();
}

bool Button::OnEvent(const sf::Event &event) {
    if (const auto *mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePressed->button != sf::Mouse::Button::Left) {
            return false;
        }

        pressed = true;
        pointerInside = true;

        RefreshVisual();

        return true;
    }

    if (const auto *mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
        if (!pressed) {
            return false;
        }

        const sf::Vector2f mousePosition{static_cast<float>(mouseMoved->position.x),
                                         static_cast<float>(mouseMoved->position.y)};

        pointerInside = Contains(mousePosition);

        RefreshVisual();

        return true;
    }

    if (const auto *mouseReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mouseReleased->button != sf::Mouse::Button::Left || !pressed) {
            return false;
        }

        const sf::Vector2f mousePosition{static_cast<float>(mouseReleased->position.x),
                                         static_cast<float>(mouseReleased->position.y)};

        const bool clicked = Contains(mousePosition);

        pointerInside = clicked;
        pressed = false;

        RefreshVisual();

        if (clicked && onClick) {
            onClick();
        }

        return true;
    }

    return false;
}

void Button::RefreshVisual() {
    if (!IsEnabled()) {
        state = ButtonState::Disabled;
        SetFillColor(disabledColor);
        return;
    }

    if (pressed && pointerInside) {
        state = ButtonState::Pressed;
        SetFillColor(pressedColor);
        return;
    }

    if (pointerInside) {
        state = ButtonState::Hovered;
        SetFillColor(hoveredColor);
        return;
    }

    state = ButtonState::Normal;
    SetFillColor(normalColor);
}
