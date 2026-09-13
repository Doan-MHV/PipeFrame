#include <PipeFrame/Backend/SFML/UI/Tooltip.h>

#include <algorithm>

Tooltip::Tooltip(const sf::Font &font, const UITheme &newTheme)
    : Surface(SurfaceVariant::Floating, newTheme), label(CreateChild<Label>(font)), theme(newTheme) {
    SetSize({180.0f, 32.0f});
    SetHitTestVisible(false);
    label.SetCharacterSize(theme.captionTextSize);
    label.SetColor(theme.textPrimary);
    SetVisible(false);
}

void Tooltip::SetText(const std::string &text) { label.SetText(text); }

void Tooltip::SetShowDelay(const float seconds) { showDelay = std::max(0.0f, seconds); }

void Tooltip::ShowAt(const sf::Vector2f anchor, const sf::FloatRect &viewport) {
    const sf::Vector2f tooltipSize = GetSize();
    sf::Vector2f position{anchor.x + 12.0f, anchor.y - tooltipSize.y - 10.0f};
    const float maximumX = std::max(viewport.position.x,
                                    viewport.position.x + viewport.size.x - tooltipSize.x);
    const float maximumY = std::max(viewport.position.y,
                                    viewport.position.y + viewport.size.y - tooltipSize.y);
    position.x = std::clamp(position.x, viewport.position.x,
                            maximumX);
    position.y = std::clamp(position.y, viewport.position.y,
                            maximumY);
    SetPosition(position);
    SetVisible(true);
    SetOpacity(0.0f);
    SetVisualOffset({0.0f, 4.0f});
    closing = false;
    opening = true;
    delayRemaining = IsReducedMotion() ? 0.0f : showDelay;
    if (delayRemaining <= 0.0f) {
        BeginOpening();
    }
}

void Tooltip::Hide() {
    opening = false;
    delayRemaining = 0.0f;
    if (!IsVisible()) {
        return;
    }
    if (IsReducedMotion()) {
        SetOpacity(0.0f);
        SetVisible(false);
        closing = false;
        return;
    }
    closing = true;
    AnimateOpacityTo(0.0f, theme.motionFast);
    AnimateVisualOffsetTo({0.0f, 2.0f}, theme.motionFast);
}

bool Tooltip::IsOpening() const { return opening; }

bool Tooltip::IsClosing() const { return closing; }

void Tooltip::OnGeometryChanged() {
    Surface::OnGeometryChanged();
    label.SetPosition({0.0f, 0.0f});
    label.SetSize(GetSize());
}

void Tooltip::OnUpdate(const float realDeltaSeconds) {
    if (opening && delayRemaining > 0.0f) {
        delayRemaining = std::max(0.0f, delayRemaining - realDeltaSeconds);
        if (delayRemaining <= 0.0f) {
            BeginOpening();
        }
    }
    if (closing && !IsMotionActive()) {
        closing = false;
        SetVisible(false);
    }
}

void Tooltip::BeginOpening() {
    opening = false;
    AnimateOpacityTo(1.0f, theme.motionNormal);
    AnimateVisualOffsetTo({0.0f, 0.0f}, theme.motionNormal);
}
