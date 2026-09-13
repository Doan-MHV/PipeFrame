#include <PipeFrame/Backend/SFML/UI/OverlayPanel.h>

#include <algorithm>
#include <limits>

namespace {

float AlignedOffset(const float available, const float childSize, const HorizontalAlignment alignment) {
    switch (alignment) {
    case HorizontalAlignment::Center:
        return (available - childSize) * 0.5f;
    case HorizontalAlignment::End:
        return available - childSize;
    case HorizontalAlignment::Start:
    case HorizontalAlignment::Stretch:
        return 0.0f;
    }
    return 0.0f;
}

float AlignedOffset(const float available, const float childSize, const VerticalAlignment alignment) {
    switch (alignment) {
    case VerticalAlignment::Center:
        return (available - childSize) * 0.5f;
    case VerticalAlignment::End:
        return available - childSize;
    case VerticalAlignment::Start:
    case VerticalAlignment::Stretch:
        return 0.0f;
    }
    return 0.0f;
}

} // namespace

void OverlayPanel::SetPadding(const Thickness newPadding) {
    padding = newPadding;
    RefreshLayout();
}

Thickness OverlayPanel::GetPadding() const { return padding; }

void OverlayPanel::SetChildAlignment(Widget &child, const Alignment alignment) {
    if (child.GetParent() != this) {
        return;
    }
    childAlignments[&child] = alignment;
    RefreshLayout();
}

Alignment OverlayPanel::GetChildAlignment(const Widget &child) const {
    const auto iterator = childAlignments.find(&child);
    return iterator == childAlignments.end() ? Alignment{} : iterator->second;
}

void OverlayPanel::RefreshLayout() {
    if (layoutInProgress) {
        return;
    }

    layoutInProgress = true;
    const sf::Vector2f measured = Measure(BoxConstraints::Unbounded());
    sf::Vector2f fittedSize = GetSize();
    if (GetWidthPolicy() == SizePolicy::FitContent) {
        fittedSize.x = measured.x;
    }
    if (GetHeightPolicy() == SizePolicy::FitContent) {
        fittedSize.y = measured.y;
    }
    if (fittedSize != GetSize()) {
        Arrange({GetPosition(), fittedSize});
    }
    ArrangeChildren();
    layoutInProgress = false;
}

void OverlayPanel::OnGeometryChanged() {
    Panel::OnGeometryChanged();
    RefreshLayout();
}

void OverlayPanel::OnChildGeometryChanged(Widget &child) {
    (void)child;
    RefreshLayout();
}

sf::Vector2f OverlayPanel::OnMeasure(const BoxConstraints &constraints) {
    const float horizontalPadding = padding.left + padding.right;
    const float verticalPadding = padding.top + padding.bottom;
    BoxConstraints childConstraints = BoxConstraints::Unbounded();
    if (constraints.HasBoundedWidth()) {
        childConstraints.maximum.x = std::max(0.0f, constraints.maximum.x - horizontalPadding);
    }
    if (constraints.HasBoundedHeight()) {
        childConstraints.maximum.y = std::max(0.0f, constraints.maximum.y - verticalPadding);
    }

    sf::Vector2f contentSize{0.0f, 0.0f};
    for (std::size_t index = 0; index < GetChildCount(); ++index) {
        Widget *child = GetChild(index);
        if (child == nullptr || !child->IsVisible()) {
            continue;
        }
        const sf::Vector2f desired = child->Measure(childConstraints);
        contentSize.x = std::max(contentSize.x, desired.x);
        contentSize.y = std::max(contentSize.y, desired.y);
    }

    return constraints.Constrain({contentSize.x + horizontalPadding, contentSize.y + verticalPadding});
}

void OverlayPanel::ArrangeChildren() {
    const sf::Vector2f panelSize = GetSize();
    const sf::Vector2f available{
        std::max(0.0f, panelSize.x - padding.left - padding.right),
        std::max(0.0f, panelSize.y - padding.top - padding.bottom),
    };
    const BoxConstraints childConstraints{{0.0f, 0.0f}, available};

    for (std::size_t index = 0; index < GetChildCount(); ++index) {
        Widget *child = GetChild(index);
        if (child == nullptr || !child->IsVisible()) {
            continue;
        }

        const sf::Vector2f desired = child->Measure(childConstraints);
        const Alignment alignment = GetChildAlignment(*child);
        const sf::Vector2f arrangedSize{
            alignment.horizontal == HorizontalAlignment::Stretch ? available.x : desired.x,
            alignment.vertical == VerticalAlignment::Stretch ? available.y : desired.y,
        };
        const sf::Vector2f position{
            padding.left + AlignedOffset(available.x, arrangedSize.x, alignment.horizontal),
            padding.top + AlignedOffset(available.y, arrangedSize.y, alignment.vertical),
        };
        child->Arrange({position, arrangedSize});
    }
}

void OverlayPanel::OnChildRemoved(Widget &child) { childAlignments.erase(&child); RefreshLayout(); }
