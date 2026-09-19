#include <PipeFrame/Backend/SFML/UI/EdgeDrawer.h>

#include <algorithm>
#include <utility>

EdgeDrawer::EdgeDrawer(const DrawerEdge newEdge, const UITheme &newTheme)
    : Surface(SurfaceVariant::Glass, newTheme), handle(CreateChild<Button>()), theme(newTheme), edge(newEdge),
      transitionDuration(newTheme.motionNormal) {
    SetSize({280.0f, 400.0f});
    SetHitTestVisible(false);
    handle.SetCornerRadius(theme.radiusSmall);
    handle.SetNormalColor(theme.controlNormal);
    handle.SetHoveredColor(theme.controlHovered);
    handle.SetPressedColor(theme.controlPressed);
    handle.SetOnClick([this]() { Toggle(); });
    RefreshHandleGeometry();
    RefreshDrawerOffset(false);
}

void EdgeDrawer::SetEdge(const DrawerEdge newEdge) {
    if (edge == newEdge) {
        return;
    }
    edge = newEdge;
    RefreshHandleGeometry();
    RefreshDrawerOffset(false);
}

DrawerEdge EdgeDrawer::GetEdge() const { return edge; }

void EdgeDrawer::SetOpen(const bool newOpen, const bool animate) {
    if (open == newOpen) {
        return;
    }
    open = newOpen;
    SetHitTestVisible(open);
    RefreshDrawerOffset(animate);
    handle.SetSelected(open);
    if (onOpenChanged) {
        onOpenChanged(open);
    }
}

bool EdgeDrawer::IsOpen() const { return open; }

void EdgeDrawer::Open(const bool animate) { SetOpen(true, animate); }

void EdgeDrawer::Close(const bool animate) { SetOpen(false, animate); }

void EdgeDrawer::Toggle() { SetOpen(!open); }

void EdgeDrawer::SetHandleExtent(const float extent) {
    handleExtent = std::max(1.0f, extent);
    RefreshHandleGeometry();
    RefreshDrawerOffset(false);
}

float EdgeDrawer::GetHandleExtent() const { return handleExtent; }

void EdgeDrawer::SetHandleLength(const float length) {
    handleLength = std::max(1.0f, length);
    RefreshHandleGeometry();
}

float EdgeDrawer::GetHandleLength() const { return handleLength; }

Button &EdgeDrawer::GetHandle() { return handle; }

const Button &EdgeDrawer::GetHandle() const { return handle; }

void EdgeDrawer::SetTransitionDuration(const float seconds) { transitionDuration = std::max(0.0f, seconds); }

void EdgeDrawer::SetOnOpenChanged(OpenChangedCallback callback) { onOpenChanged = std::move(callback); }

void EdgeDrawer::OnGeometryChanged() {
    Surface::OnGeometryChanged();
    RefreshHandleGeometry();
    if (!IsMotionActive()) {
        RefreshDrawerOffset(false);
    }
}

void EdgeDrawer::OnRender(sf::RenderTarget &target) const {
    // A collapsed drawer is represented solely by its handle. Rendering the
    // translated surface leaves a rounded strip along the viewport edge.
    if (open) {
        Surface::OnRender(target);
    }
}

sf::Vector2f EdgeDrawer::ClosedOffset() const {
    const sf::Vector2f size = GetSize();
    switch (edge) {
    case DrawerEdge::Left:
        return {-std::max(0.0f, size.x - handleExtent), 0.0f};
    case DrawerEdge::Right:
        return {std::max(0.0f, size.x - handleExtent), 0.0f};
    case DrawerEdge::Top:
        return {0.0f, -std::max(0.0f, size.y - handleExtent)};
    case DrawerEdge::Bottom:
        return {0.0f, std::max(0.0f, size.y - handleExtent)};
    }
    return {};
}

void EdgeDrawer::RefreshHandleGeometry() {
    const sf::Vector2f size = GetSize();
    const float verticalLength = std::min(handleLength, size.y);
    const float horizontalLength = std::min(handleLength, size.x);
    switch (edge) {
    case DrawerEdge::Left:
        handle.SetPosition({std::max(0.0f, size.x - handleExtent), std::max(0.0f, (size.y - verticalLength) * 0.5f)});
        handle.SetSize({handleExtent, verticalLength});
        break;
    case DrawerEdge::Right:
        handle.SetPosition({0.0f, std::max(0.0f, (size.y - verticalLength) * 0.5f)});
        handle.SetSize({handleExtent, verticalLength});
        break;
    case DrawerEdge::Top:
        handle.SetPosition({std::max(0.0f, (size.x - horizontalLength) * 0.5f), std::max(0.0f, size.y - handleExtent)});
        handle.SetSize({horizontalLength, handleExtent});
        break;
    case DrawerEdge::Bottom:
        handle.SetPosition({std::max(0.0f, (size.x - horizontalLength) * 0.5f), 0.0f});
        handle.SetSize({horizontalLength, handleExtent});
        break;
    }
}

void EdgeDrawer::RefreshDrawerOffset(const bool animate) {
    const sf::Vector2f target = open ? sf::Vector2f{} : ClosedOffset();
    if (animate && !IsReducedMotion()) {
        AnimateVisualOffsetTo(target, transitionDuration);
    } else {
        SetVisualOffset(target);
    }
}
