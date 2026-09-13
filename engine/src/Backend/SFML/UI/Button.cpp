#include <PipeFrame/Backend/SFML/UI/Button.h>

#include <algorithm>
#include <utility>

#include <SFML/Window/Mouse.hpp>

#include <PipeFrame/Backend/SFML/UI/UITheme.h>

Button::Button() {
    const UITheme &theme = UITheme::Dark();
    SetOutlineColor(sf::Color(76, 84, 102));
    SetOutlineThickness(1.0f);
    SetCornerRadius(theme.radiusSmall);
    SetTransitionDuration(theme.motionFast);
    SetFocusable(true);

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

void Button::SetSelectedColor(const sf::Color color) {
    selectedColor = color;
    RefreshVisual();
}

void Button::SetFocusedColor(const sf::Color color) {
    focusedColor = color;
    RefreshVisual();
}

void Button::SetSelected(const bool newSelected) {
    if (selected == newSelected) {
        return;
    }
    selected = newSelected;
    RefreshVisual();
}

bool Button::IsSelected() const { return selected; }

void Button::OnGeometryChanged() {
    Panel::OnGeometryChanged();
    for (std::size_t index = 0; index < GetChildCount(); ++index) {
        if (Widget *child = GetChild(index)) {
            child->Arrange({{}, GetSize()});
        }
    }
}

void Button::SetTransitionDuration(const float seconds) {
    transitionDuration = std::max(0.0f, seconds);
}

void Button::SetReducedMotion(const bool newReducedMotion) {
    reducedMotion = newReducedMotion;
    Widget::SetReducedMotion(newReducedMotion);
    if (reducedMotion) {
        animatedColor.SetTarget(animatedColor.GetTarget(), true);
        SetFillColor(animatedColor.Get());
    }
}

sf::Color Button::GetVisualColor() const { return animatedColor.Get(); }

ButtonState Button::GetState() const { return state; }

void Button::OnPointerEntered() {
    pointerInside = true;
    AnimateVisualOffsetTo({0.0f, -1.0f}, transitionDuration);
    RefreshVisual();
}

void Button::OnPointerExited() {
    pointerInside = false;
    AnimateVisualOffsetTo({0.0f, 0.0f}, transitionDuration);
    RefreshVisual();
}

void Button::OnEnabledChanged() {
    pressed = false;
    keyboardPressed = false;
    AnimateVisualOffsetTo({0.0f, 0.0f}, transitionDuration);
    RefreshVisual();
}

void Button::OnUpdate(const float realDeltaSeconds) {
    if (animatedColor.Update(realDeltaSeconds, reducedMotion ? 0.0f : transitionDuration)) {
        SetFillColor(animatedColor.Get());
    }
}

void Button::OnKeyboardFocusGained() { RefreshVisual(); }

void Button::OnKeyboardFocusLost() {
    keyboardPressed = false;
    pressed = false;
    AnimateVisualOffsetTo(pointerInside ? sf::Vector2f{0.0f, -1.0f}
                                        : sf::Vector2f{0.0f, 0.0f},
                          transitionDuration);
    RefreshVisual();
}

bool Button::OnEvent(const sf::Event &event) {
    if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code != sf::Keyboard::Key::Enter && keyPressed->code != sf::Keyboard::Key::Space) {
            return false;
        }
        keyboardPressed = true;
        pressed = true;
        AnimateVisualOffsetTo({0.0f, 0.0f}, transitionDuration);
        RefreshVisual();
        return true;
    }

    if (const auto *keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        if (keyReleased->code != sf::Keyboard::Key::Enter && keyReleased->code != sf::Keyboard::Key::Space) {
            return false;
        }
        const bool activate = keyboardPressed;
        keyboardPressed = false;
        pressed = false;
        AnimateVisualOffsetTo(pointerInside ? sf::Vector2f{0.0f, -1.0f}
                                            : sf::Vector2f{0.0f, 0.0f},
                              transitionDuration);
        RefreshVisual();
        if (activate && onClick) {
            onClick();
        }
        return true;
    }

    if (const auto *mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePressed->button != sf::Mouse::Button::Left) {
            return false;
        }

        pressed = true;
        pointerInside = true;
        AnimateVisualOffsetTo({0.0f, 0.0f}, transitionDuration);

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
        AnimateVisualOffsetTo(clicked ? sf::Vector2f{0.0f, -1.0f}
                                      : sf::Vector2f{0.0f, 0.0f},
                              transitionDuration);

        RefreshVisual();

        if (clicked && onClick) {
            onClick();
        }

        return true;
    }

    return false;
}

void Button::RefreshVisual() {
    sf::Color targetColor = normalColor;

    if (!IsEnabled()) {
        state = ButtonState::Disabled;
        targetColor = disabledColor;
    } else if (pressed && (pointerInside || keyboardPressed)) {
        state = ButtonState::Pressed;
        targetColor = pressedColor;
    } else if (pointerInside) {
        state = ButtonState::Hovered;
        targetColor = hoveredColor;
    } else if (selected) {
        state = ButtonState::Selected;
        targetColor = selectedColor;
    } else if (HasKeyboardFocus()) {
        state = ButtonState::Focused;
        targetColor = focusedColor;
    } else {
        state = ButtonState::Normal;
    }

    animatedColor.SetTarget(targetColor, reducedMotion);
    SetFillColor(animatedColor.Get());
}
