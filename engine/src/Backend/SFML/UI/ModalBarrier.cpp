#include <PipeFrame/Backend/SFML/UI/ModalBarrier.h>

#include <utility>

#include <SFML/Window/Mouse.hpp>

ModalBarrier::ModalBarrier(const UITheme &theme) {
    SetFillColor(theme.scrim);
    SetOutlineColor(sf::Color::Transparent);
    SetOutlineThickness(0.0f);
    SetFocusable(true);
    SetInputBarrier(true);
}

void ModalBarrier::SetDismissOnBackgroundClick(const bool dismiss) {
    dismissOnBackgroundClick = dismiss;
}

bool ModalBarrier::DismissesOnBackgroundClick() const { return dismissOnBackgroundClick; }

void ModalBarrier::SetOnDismiss(DismissCallback callback) { onDismiss = std::move(callback); }

bool ModalBarrier::OnEvent(const sf::Event &event) {
    const auto background = [&](sf::Vector2i point) {
        for (std::size_t i = 0; i < GetChildCount(); ++i) {
            const auto *child = GetChild(i);
            if (child->IsVisible() && child->Contains(sf::Vector2f(point))) return false;
        }
        return Contains(sf::Vector2f(point));
    };
    if (const auto *key = event.getIf<sf::Event::KeyPressed>(); key && key->code == sf::Keyboard::Key::Escape) {
        if (onDismiss) onDismiss();
        return true;
    }
    if (const auto *pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (pressed->button == sf::Mouse::Button::Left) {
            pointerPressed = background(pressed->position);
        }
        return true;
    }
    if (const auto *released = event.getIf<sf::Event::MouseButtonReleased>()) {
        const bool dismiss = pointerPressed && released->button == sf::Mouse::Button::Left &&
                             dismissOnBackgroundClick && background(released->position);
        pointerPressed = false;
        if (dismiss && onDismiss) {
            onDismiss();
        }
        return true;
    }
    if (event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseWheelScrolled>()) {
        return true;
    }
    return false;
}
