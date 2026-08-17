#include <PipeFrame/UI/UIManager.h>

bool UIManager::HandleEvent(const sf::Event &event) {
    const std::optional<sf::Vector2f> pointerPosition = GetPointerPosition(event);

    // Keyboard focus will be implemented later.
    if (!pointerPosition) {
        return false;
    }

    Widget *hitWidget = FindTopmostAt(*pointerPosition);

    if (event.is<sf::Event::MouseMoved>()) {
        UpdateHoveredWidget(hitWidget);

        Widget *target = capturedWidget != nullptr ? capturedWidget : hitWidget;

        if (target == nullptr) {
            return false;
        }

        DispatchEvent(target, event);
        return true;
    }

    if (event.is<sf::Event::MouseButtonPressed>()) {
        UpdateHoveredWidget(hitWidget);

        if (hitWidget == nullptr) {
            return false;
        }

        // Future movement and release events go back to this widget.
        capturedWidget = hitWidget;

        DispatchEvent(capturedWidget, event);
        return true;
    }

    if (event.is<sf::Event::MouseButtonReleased>()) {
        UpdateHoveredWidget(hitWidget);

        Widget *target = capturedWidget != nullptr ? capturedWidget : hitWidget;

        if (target == nullptr) {
            return false;
        }

        DispatchEvent(target, event);

        capturedWidget = nullptr;

        return true;
    }

    if (event.is<sf::Event::MouseWheelScrolled>()) {
        if (hitWidget == nullptr) {
            return false;
        }

        DispatchEvent(hitWidget, event);
        return true;
    }

    return false;
}

void UIManager::UpdateHoveredWidget(Widget *newHoveredWidget) {
    if (hoveredWidget == newHoveredWidget) {
        return;
    }

    if (hoveredWidget != nullptr) {
        hoveredWidget->OnPointerExited();
    }

    hoveredWidget = newHoveredWidget;

    if (hoveredWidget != nullptr) {
        hoveredWidget->OnPointerEntered();
    }
}

void UIManager::DispatchEvent(Widget *target, const sf::Event &event) {
    Widget *currentWidget = target;

    while (currentWidget != nullptr) {
        if (currentWidget->OnEvent(event)) {
            return;
        }

        currentWidget = currentWidget->GetParent();
    }
}

void UIManager::Render(sf::RenderTarget &target) const {
    for (const std::unique_ptr<Widget> &root : roots) {
        root->Render(target);
    }
}

std::size_t UIManager::GetRootCount() const { return roots.size(); }

std::optional<sf::Vector2f> UIManager::GetPointerPosition(const sf::Event &event) {
    if (const auto *moved = event.getIf<sf::Event::MouseMoved>()) {
        return sf::Vector2f{static_cast<float>(moved->position.x), static_cast<float>(moved->position.y)};
    }

    if (const auto *pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        return sf::Vector2f{static_cast<float>(pressed->position.x), static_cast<float>(pressed->position.y)};
    }

    if (const auto *released = event.getIf<sf::Event::MouseButtonReleased>()) {
        return sf::Vector2f{static_cast<float>(released->position.x), static_cast<float>(released->position.y)};
    }

    if (const auto *wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        return sf::Vector2f{static_cast<float>(wheel->position.x), static_cast<float>(wheel->position.y)};
    }

    return std::nullopt;
}

Widget *UIManager::FindTopmostAt(sf::Vector2f screenPoint) {
    // The last root is visually on top.
    for (auto iterator = roots.rbegin(); iterator != roots.rend(); ++iterator) {
        Widget *hitWidget = (*iterator)->FindTopmostAt(screenPoint);

        if (hitWidget != nullptr) {
            return hitWidget;
        }
    }

    return nullptr;
}