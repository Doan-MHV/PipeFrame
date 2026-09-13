#include <PipeFrame/Backend/SFML/UI/PopupLayer.h>

#include <algorithm>
#include <utility>

#include <SFML/Window/Mouse.hpp>

namespace {

sf::Vector2f PointerPosition(const sf::Event &event) {
    if (const auto *pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        return {static_cast<float>(pressed->position.x), static_cast<float>(pressed->position.y)};
    }
    if (const auto *released = event.getIf<sf::Event::MouseButtonReleased>()) {
        return {static_cast<float>(released->position.x), static_cast<float>(released->position.y)};
    }
    if (const auto *moved = event.getIf<sf::Event::MouseMoved>()) {
        return {static_cast<float>(moved->position.x), static_cast<float>(moved->position.y)};
    }
    if (const auto *wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        return {static_cast<float>(wheel->position.x), static_cast<float>(wheel->position.y)};
    }
    return {};
}

} // namespace

PopupLayer::PopupLayer(const UITheme &newTheme)
    : content(CreateChild<Surface>(SurfaceVariant::Floating, newTheme)), theme(newTheme) {
    SetFillColor(theme.scrim);
    SetOutlineColor(sf::Color::Transparent);
    SetOutlineThickness(0.0f);
    SetFocusable(true);
    SetInputBarrier(true);
    SetVisible(false);
}

Surface &PopupLayer::GetContent() { return content; }

const Surface &PopupLayer::GetContent() const { return content; }

void PopupLayer::SetContentBounds(const sf::FloatRect &bounds) {
    requestedContentBounds = bounds;
    requestedContentBounds.size.x = std::max(0.0f, requestedContentBounds.size.x);
    requestedContentBounds.size.y = std::max(0.0f, requestedContentBounds.size.y);
    ArrangeContent();
}

sf::FloatRect PopupLayer::GetContentBounds() const {
    return {content.GetPosition(), content.GetSize()};
}

void PopupLayer::SetDismissOnBackgroundClick(const bool dismiss) {
    dismissOnBackgroundClick = dismiss;
}

bool PopupLayer::DismissesOnBackgroundClick() const { return dismissOnBackgroundClick; }

void PopupLayer::SetOnDismiss(DismissCallback callback) { onDismiss = std::move(callback); }

void PopupLayer::Open() {
    closing = false;
    open = true;
    backgroundPointerPressed = false;
    SetEnabled(true);
    SetVisible(true);
    SetOpacity(0.0f);
    content.SetVisualOffset({0.0f, theme.spacing8});
    AnimateOpacityTo(1.0f, theme.motionFast);
    content.AnimateVisualOffsetTo({0.0f, 0.0f}, theme.motionNormal);
}

void PopupLayer::Dismiss() {
    if (!open || closing) {
        return;
    }
    closing = true;
    backgroundPointerPressed = false;
    SetEnabled(false);
    AnimateOpacityTo(0.0f, theme.motionFast);
    content.AnimateVisualOffsetTo({0.0f, theme.spacing4}, theme.motionFast);
    if (IsReducedMotion()) {
        OnUpdate(0.0f);
    }
}

bool PopupLayer::IsOpen() const { return open && !closing; }

bool PopupLayer::OnEvent(const sf::Event &event) {
    if (const auto *key = event.getIf<sf::Event::KeyPressed>(); key && key->code == sf::Keyboard::Key::Escape) {
        Dismiss();
        return true;
    }
    if (const auto *pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        backgroundPointerPressed =
            pressed->button == sf::Mouse::Button::Left && !content.Contains(PointerPosition(event));
        return true;
    }
    if (const auto *released = event.getIf<sf::Event::MouseButtonReleased>()) {
        const bool dismiss = released->button == sf::Mouse::Button::Left &&
                             backgroundPointerPressed &&
                             !content.Contains(PointerPosition(event)) &&
                             dismissOnBackgroundClick;
        backgroundPointerPressed = false;
        if (dismiss) {
            Dismiss();
        }
        return true;
    }
    if (event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseWheelScrolled>()) {
        return true;
    }
    return false;
}

void PopupLayer::OnGeometryChanged() {
    Panel::OnGeometryChanged();
    ArrangeContent();
}

void PopupLayer::OnUpdate(const float realDeltaSeconds) {
    (void)realDeltaSeconds;
    if (!closing || IsMotionActive() || content.IsMotionActive()) {
        return;
    }
    closing = false;
    open = false;
    SetVisible(false);
    if (onDismiss) {
        onDismiss();
    }
}

void PopupLayer::ArrangeContent() {
    const sf::Vector2f viewport = GetSize();
    const sf::Vector2f contentSize{
        std::min(requestedContentBounds.size.x, viewport.x),
        std::min(requestedContentBounds.size.y, viewport.y),
    };
    const sf::Vector2f maximumPosition{
        std::max(0.0f, viewport.x - contentSize.x),
        std::max(0.0f, viewport.y - contentSize.y),
    };
    const sf::Vector2f contentPosition{
        std::clamp(requestedContentBounds.position.x, 0.0f, maximumPosition.x),
        std::clamp(requestedContentBounds.position.y, 0.0f, maximumPosition.y),
    };
    content.Arrange({contentPosition, contentSize});
}
