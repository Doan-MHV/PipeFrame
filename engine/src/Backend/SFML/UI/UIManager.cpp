#include <PipeFrame/Backend/SFML/UI/UIManager.h>

#include <algorithm>

namespace {
bool IsWithin(const Widget *widget, const Widget *ancestor) {
    for (; widget; widget = widget->GetParent())
        if (widget == ancestor)
            return true;
    return false;
}
Widget *FindBarrier(Widget &widget) {
    if (!widget.IsVisible() || !widget.IsEnabled())
        return nullptr;
    for (std::size_t i = widget.GetChildCount(); i > 0; --i)
        if (auto *barrier = FindBarrier(*widget.GetChild(i - 1)))
            return barrier;
    return widget.IsInputBarrier() ? &widget : nullptr;
}
void CollectFocusable(Widget &widget, std::vector<Widget *> &result) {
    if (!widget.IsVisible() || !widget.IsEnabled())
        return;
    if (widget.IsFocusable())
        result.push_back(&widget);
    for (std::size_t i = 0; i < widget.GetChildCount(); ++i)
        CollectFocusable(*widget.GetChild(i), result);
}
} // namespace

bool UIManager::HandleEvent(const sf::Event &event) {
    Update(0);
    PruneExpiredInput();
    if (event.is<sf::Event::FocusLost>()) {
        SetKeyboardFocus(nullptr);
        return false;
    }

    if (!IsInteractiveInTree(hoveredWidget)) {
        UpdateHoveredWidget(nullptr);
    }
    if (!IsInteractiveInTree(capturedWidget)) {
        capturedWidget = nullptr;
    }
    if (!IsInteractiveInTree(focusedWidget)) {
        SetKeyboardFocus(nullptr);
    }

    Widget *barrier = nullptr;
    for (auto it = roots.rbegin(); it != roots.rend() && !barrier; ++it)
        barrier = FindBarrier(**it);
    if (barrier) {
        if (!IsWithin(capturedWidget, barrier))
            capturedWidget = nullptr;
        if (!IsWithin(hoveredWidget, barrier))
            UpdateHoveredWidget(nullptr);
        if (!IsWithin(focusedWidget, barrier))
            SetKeyboardFocus(FindFocusableAncestor(barrier));
    }
    if (const auto *key = event.getIf<sf::Event::KeyPressed>(); key && key->code == sf::Keyboard::Key::Tab) {
        std::vector<Widget *> candidates;
        if (barrier)
            CollectFocusable(*barrier, candidates);
        else
            for (auto &root : roots)
                CollectFocusable(*root, candidates);
        if (candidates.empty())
            return barrier != nullptr;
        auto it = std::find(candidates.begin(), candidates.end(), focusedWidget);
        std::size_t index = key->shift ? candidates.size() - 1 : 0;
        if (it != candidates.end()) {
            const auto current = static_cast<std::size_t>(it - candidates.begin());
            index =
                key->shift ? (current + candidates.size() - 1) % candidates.size() : (current + 1) % candidates.size();
        }
        SetKeyboardFocus(candidates[index]);
        return true;
    }

    const std::optional<sf::Vector2f> pointerPosition = GetPointerPosition(event);

    if (!pointerPosition) {
        if (focusedWidget == nullptr) {
            return barrier != nullptr;
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
    if (barrier && !IsWithin(hitWidget, barrier))
        hitWidget = barrier;

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
        capturedLifetime = hitWidget->Lifetime();

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
    hoveredLifetime = hoveredWidget ? hoveredWidget->Lifetime() : std::weak_ptr<void>{};

    if (hoveredWidget != nullptr) {
        hoveredWidget->OnPointerEntered();
    }
}

void UIManager::DispatchEvent(Widget *target, const sf::Event &event) {
    Widget *currentWidget = target;

    while (currentWidget != nullptr) {
        const auto lifetime = currentWidget->Lifetime();
        if (currentWidget->OnEvent(event) || lifetime.expired()) {
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

void UIManager::Update(const float realDeltaSeconds) {
    if (updating)
        throw std::logic_error("UI updates cannot be nested");
    updating = true;
    const auto finish = [&] {
        updating = false;
        for (auto &root : pendingRoots)
            roots.push_back(std::move(root));
        pendingRoots.clear();
    };
    try {
        PruneExpiredInput();
        for (const auto &root : roots)
            root->Update(realDeltaSeconds);
        std::erase_if(roots, [](const auto &root) { return root->IsDisposed(); });
        PruneExpiredInput();
        if (!IsInteractiveInTree(hoveredWidget))
            UpdateHoveredWidget(nullptr);
        if (!IsInteractiveInTree(capturedWidget))
            capturedWidget = nullptr;
        if (!IsInteractiveInTree(focusedWidget))
            SetKeyboardFocus(nullptr);
    } catch (...) {
        finish();
        throw;
    }
    finish();
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

bool UIManager::IsInteractiveInTree(const Widget *widget) {
    const Widget *current = widget;
    if (current == nullptr) {
        return false;
    }

    while (current != nullptr) {
        if (!current->IsVisible() || !current->IsEnabled()) {
            return false;
        }
        current = current->GetParent();
    }
    return true;
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
    focusedLifetime = focusedWidget ? focusedWidget->Lifetime() : std::weak_ptr<void>{};

    if (focusedWidget != nullptr) {
        focusedWidget->keyboardFocused = true;
        focusedWidget->OnKeyboardFocusGained();
    }
}

bool UIManager::HasKeyboardFocus() const { return focusedWidget != nullptr && !focusedLifetime.expired(); }
void UIManager::PruneExpiredInput() {
    if (hoveredLifetime.expired())
        hoveredWidget = nullptr;
    if (capturedLifetime.expired())
        capturedWidget = nullptr;
    if (focusedLifetime.expired())
        focusedWidget = nullptr;
}
