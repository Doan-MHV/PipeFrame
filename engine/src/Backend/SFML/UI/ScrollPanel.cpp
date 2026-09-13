#include <PipeFrame/Backend/SFML/UI/ScrollPanel.h>

#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <cmath>

void ScrollPanel::SetContent(Widget &newContent) {

    if (newContent.GetParent() != this) {
        return;
    }

    content = &newContent;
    RefreshContentPosition();
}

Widget *ScrollPanel::GetContent() { return content; }

const Widget *ScrollPanel::GetContent() const { return content; }

void ScrollPanel::SetScrollOffset(const float newOffset) {

    const float safeOffset = std::isfinite(newOffset) ? newOffset : 0.0f;

    scrollOffset = std::clamp(safeOffset, 0.0f, GetMaximumScrollOffset());

    RefreshContentPosition();
}

float ScrollPanel::GetScrollOffset() const { return scrollOffset; }

void ScrollPanel::SetWheelStep(const float newWheelStep) {

    if (!std::isfinite(newWheelStep)) {
        return;
    }

    wheelStep = std::max(0.0f, newWheelStep);
}

float ScrollPanel::GetWheelStep() const { return wheelStep; }

float ScrollPanel::GetMaximumScrollOffset() const {
    if (content == nullptr) {
        return 0.0f;
    }

    return std::max(0.0f, content->GetSize().y - GetSize().y);
}

void ScrollPanel::ScrollBy(const float delta) { SetScrollOffset(scrollOffset + delta); }

void ScrollPanel::Render(sf::RenderTarget &target) const {

    if (!IsVisible()) {
        return;
    }

    OnRender(target);

    const sf::Vector2u targetSize = target.getSize();

    if (targetSize.x == 0 || targetSize.y == 0) {
        return;
    }

    const sf::View previousView = target.getView();

    sf::View clippedView = previousView;

    const sf::Vector2f screenPosition = GetScreenPosition();

    const sf::Vector2f panelSize = GetSize();

    const auto start = target.mapCoordsToPixel(screenPosition);
    const auto end = target.mapCoordsToPixel(screenPosition + panelSize);
    const sf::FloatRect panelScissor{
        {static_cast<float>(start.x) / targetSize.x, static_cast<float>(start.y) / targetSize.y},
        {static_cast<float>(std::max(0,end.x-start.x)) / targetSize.x,
         static_cast<float>(std::max(0,end.y-start.y)) / targetSize.y}};

    const auto intersection = previousView.getScissor().findIntersection(panelScissor);

    clippedView.setScissor(intersection.value_or(sf::FloatRect{
        {0.0f, 0.0f},
        {0.0f, 0.0f},
    }));

    target.setView(clippedView);
    RenderChildren(target);
    // A shared overflow indicator also serves declarative Scroll views. It stays
    // inside the viewport and follows its clipping; it is not an input control.
    const float maximum = GetMaximumScrollOffset();
    if (maximum > 0 && panelSize.x >= 8 && panelSize.y > 8) {
        const float trackHeight = panelSize.y - 8;
        const float thumbHeight = std::min(trackHeight, std::max(24.0f,
            trackHeight * panelSize.y / (panelSize.y + maximum)));
        sf::RectangleShape indicator({4, trackHeight});
        indicator.setPosition(screenPosition + sf::Vector2f{panelSize.x - 6, 4});
        indicator.setFillColor(sf::Color{65, 73, 88, static_cast<std::uint8_t>(180 * GetOpacity())});
        target.draw(indicator);
        indicator.setSize({4, thumbHeight});
        indicator.move({0, (trackHeight - thumbHeight) * scrollOffset / maximum});
        indicator.setFillColor(sf::Color{155, 176, 200, static_cast<std::uint8_t>(255 * GetOpacity())});
        target.draw(indicator);
    }
    target.setView(previousView);
}

bool ScrollPanel::OnEvent(const sf::Event &event) {

    const auto *wheel = event.getIf<sf::Event::MouseWheelScrolled>();

    if (wheel == nullptr) {
        return false;
    }

    if (wheel->wheel != sf::Mouse::Wheel::Vertical) {
        return false;
    }

    const float previousOffset = scrollOffset;
    ScrollBy(-wheel->delta * wheelStep);
    // A short list or a list at its edge must let its enclosing page scroll.
    // UIManager bubbles unhandled events through the widget ancestors.
    return scrollOffset != previousOffset;
}

void ScrollPanel::OnGeometryChanged() {
    Panel::OnGeometryChanged();
    SetScrollOffset(scrollOffset);
}

void ScrollPanel::OnChildGeometryChanged(Widget &child) {

    if (&child == content) {
        SetScrollOffset(scrollOffset);
    }
}

bool ScrollPanel::ClipsChildren() const { return true; }

void ScrollPanel::RefreshContentPosition() {
    if (content == nullptr) {
        return;
    }

    content->SetPosition({
        0.0f,
        -scrollOffset,
    });
}

void ScrollPanel::OnChildRemoved(Widget &child) { if (content == &child) { content = nullptr; scrollOffset = 0; } }
