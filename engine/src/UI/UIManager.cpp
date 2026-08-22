#include <PipeFrame/UI/UIManager.h>

bool UIManager::HandleEvent(const sf::Event &event) {
    if (event.is<sf::Event::FocusLost>()) {
        SetKeyboardFocus(nullptr);
        return false;
    }

    const std::optional<sf::Vector2f> pointerPosition = GetPointerPosition(event);

    if (!pointerPosition) {
        if (focusedWidget == nullptr) {
            return false;
        }

        const bool keyboardEvent = event.is<sf::Event::TextEntered>() || event.is<sf::Event::KeyPressed>() ||
                                   event.is<sf::Event::KeyReleased>();

        if (!keyboardEvent) {
            return false;
        }

        DispatchEvent(focusedWidget, event);
        return true;
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

        SetKeyboardFocus(FindFocusableAncestor(hitWidget));

        if (hitWidget == nullptr) {
            return false;
        }

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

bool UIManager::LoadDefaultFont(const std::filesystem::path &fontPath) {
    defaultFontLoaded = defaultFont.openFromFile(fontPath);
    return defaultFontLoaded;
}

bool UIManager::HasDefaultFont() const { return defaultFontLoaded; }

const sf::Font &UIManager::GetDefaultFont() const { return defaultFont; }

Widget *UIManager::FindFocusableAncestor(Widget *widget) {
    Widget *current = widget;

    while (current != nullptr) {
        if (current->IsFocusable()) {
            return current;
        }

        current = current->GetParent();
    }

    return nullptr;
}

void UIManager::SetKeyboardFocus(Widget *widget) {
    if (focusedWidget == widget) {
        return;
    }

    if (focusedWidget != nullptr) {
        focusedWidget->keyboardFocused = false;
        focusedWidget->OnKeyboardFocusLost();
    }

    focusedWidget = widget;

    if (focusedWidget != nullptr) {
        focusedWidget->keyboardFocused = true;
        focusedWidget->OnKeyboardFocusGained();
    }
}

bool UIManager::HasKeyboardFocus() const { return focusedWidget != nullptr; }