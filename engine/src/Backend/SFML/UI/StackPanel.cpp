#include <PipeFrame/Backend/SFML/UI/StackPanel.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

std::vector<Widget *> GetVisibleChildren(StackPanel &panel) {

    std::vector<Widget *> children;

    children.reserve(panel.GetChildCount());

    for (std::size_t index = 0; index < panel.GetChildCount(); ++index) {

        Widget *child = panel.GetChild(index);

        if (child != nullptr && child->IsVisible()) {
            children.push_back(child);
        }
    }

    return children;
}

float EffectiveFlex(const StackPanel &panel, const Widget &child, const StackOrientation orientation) {
    const float explicitFlex = panel.GetChildFlex(child);
    if (explicitFlex > 0.0f) {
        return explicitFlex;
    }

    const SizePolicy mainPolicy = orientation == StackOrientation::Vertical ? child.GetHeightPolicy()
                                                                            : child.GetWidthPolicy();
    return mainPolicy == SizePolicy::Stretch ? 1.0f : 0.0f;
}

BoxConstraints ChildMeasureConstraints(const StackOrientation orientation, const float availableCrossAxis) {
    BoxConstraints constraints = BoxConstraints::Unbounded();
    if (orientation == StackOrientation::Vertical) {
        constraints.maximum.x = availableCrossAxis;
    } else {
        constraints.maximum.y = availableCrossAxis;
    }
    return constraints;
}

} // namespace

void StackPanel::SetOrientation(const StackOrientation newOrientation) {

    if (orientation == newOrientation) {
        return;
    }

    orientation = newOrientation;
    RefreshLayout();
}

StackOrientation StackPanel::GetOrientation() const { return orientation; }

void StackPanel::SetPadding(const Thickness newPadding) {

    padding = newPadding;
    RefreshLayout();
}

Thickness StackPanel::GetPadding() const { return padding; }

void StackPanel::SetSpacing(const float newSpacing) {

    spacing = std::max(0.0f, newSpacing);
    RefreshLayout();
}

float StackPanel::GetSpacing() const { return spacing; }

void StackPanel::SetMainAxisAlignment(const MainAxisAlignment newAlignment) {

    if (mainAxisAlignment == newAlignment) {
        return;
    }

    mainAxisAlignment = newAlignment;
    RefreshLayout();
}

MainAxisAlignment StackPanel::GetMainAxisAlignment() const { return mainAxisAlignment; }

void StackPanel::SetCrossAxisAlignment(const CrossAxisAlignment newAlignment) {

    if (crossAxisAlignment == newAlignment) {
        return;
    }

    crossAxisAlignment = newAlignment;
    RefreshLayout();
}

CrossAxisAlignment StackPanel::GetCrossAxisAlignment() const { return crossAxisAlignment; }

void StackPanel::SetChildFlex(Widget &child, const float flex) {

    if (child.GetParent() != this) {
        return;
    }

    const float safeFlex = std::isfinite(flex) ? std::max(0.0f, flex) : 0.0f;

    if (safeFlex == 0.0f) {
        childFlex.erase(&child);
    } else {
        childFlex[&child] = safeFlex;
    }

    RefreshLayout();
}

float StackPanel::GetChildFlex(const Widget &child) const {

    const auto iterator = childFlex.find(&child);

    if (iterator == childFlex.end()) {
        return 0.0f;
    }

    return iterator->second;
}

void StackPanel::RefreshLayout() {
    if (layoutInProgress) {
        return;
    }

    layoutInProgress = true;

    auto constraints = BoxConstraints::Unbounded();
    if (GetWidthPolicy() != SizePolicy::FitContent && GetSize().x > 0) constraints.maximum.x = GetSize().x;
    const sf::Vector2f measured = Measure(constraints);
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

    if (orientation == StackOrientation::Vertical) {
        LayoutVertical();
    } else {
        LayoutHorizontal();
    }

    layoutInProgress = false;
}

void StackPanel::OnGeometryChanged() {
    Panel::OnGeometryChanged();
    RefreshLayout();
}

void StackPanel::OnChildGeometryChanged(Widget &child) {

    (void)child;
    RefreshLayout();
}

sf::Vector2f StackPanel::OnMeasure(const BoxConstraints &constraints) {
    const std::vector<Widget *> children = GetVisibleChildren(*this);
    const float horizontalPadding = padding.left + padding.right;
    const float verticalPadding = padding.top + padding.bottom;
    const float maximumContentWidth = constraints.HasBoundedWidth()
                                          ? std::max(0.0f, constraints.maximum.x - horizontalPadding)
                                          : std::numeric_limits<float>::infinity();
    const float maximumContentHeight = constraints.HasBoundedHeight()
                                           ? std::max(0.0f, constraints.maximum.y - verticalPadding)
                                           : std::numeric_limits<float>::infinity();
    const BoxConstraints childConstraints = ChildMeasureConstraints(
        orientation, orientation == StackOrientation::Vertical ? maximumContentWidth : maximumContentHeight);

    float horizontalFixed=0.0f, horizontalFlex=0.0f;
    if (orientation==StackOrientation::Horizontal && std::isfinite(maximumContentWidth)) {
        for (Widget *child : children) {
            const float flex=EffectiveFlex(*this,*child,orientation);
            if (flex>0) horizontalFlex+=flex;
            else horizontalFixed+=child->Measure(childConstraints).x;
        }
    }
    const float horizontalRemaining=std::max(0.0f,maximumContentWidth-horizontalFixed-
        spacing*static_cast<float>(children.empty() ? 0 : children.size()-1));
    float mainSize = 0.0f;
    float crossSize = 0.0f;

    for (Widget *child : children) {
        auto measuredConstraints=childConstraints;
        if (orientation==StackOrientation::Horizontal && horizontalFlex>0) {
            const float flex=EffectiveFlex(*this,*child,orientation);
            if (flex>0) measuredConstraints.maximum.x=horizontalRemaining*flex/horizontalFlex;
        }
        const sf::Vector2f childSize = child->Measure(measuredConstraints);
        if (orientation == StackOrientation::Vertical) {
            mainSize += childSize.y;
            crossSize = std::max(crossSize, childSize.x);
        } else {
            mainSize += childSize.x;
            crossSize = std::max(crossSize, childSize.y);
        }
    }

    if (children.size() > 1) {
        mainSize += spacing * static_cast<float>(children.size() - 1);
    }

    const sf::Vector2f measured = orientation == StackOrientation::Vertical
                                      ? sf::Vector2f{crossSize + horizontalPadding, mainSize + verticalPadding}
                                      : sf::Vector2f{mainSize + horizontalPadding, crossSize + verticalPadding};
    return constraints.Constrain(measured);
}

float StackPanel::CalculateLeadingOffset(const float unusedSpace, const std::size_t visibleChildCount) const {

    if (unusedSpace <= 0.0f || visibleChildCount == 0) {
        return 0.0f;
    }

    switch (mainAxisAlignment) {
    case MainAxisAlignment::Center:
        return unusedSpace * 0.5f;

    case MainAxisAlignment::End:
        return unusedSpace;

    case MainAxisAlignment::Start:
    case MainAxisAlignment::SpaceBetween:
        return 0.0f;
    }

    return 0.0f;
}

float StackPanel::CalculateSpacing(const float unusedSpace, const std::size_t visibleChildCount) const {

    if (mainAxisAlignment != MainAxisAlignment::SpaceBetween || visibleChildCount <= 1 || unusedSpace <= 0.0f) {
        return spacing;
    }

    return spacing + unusedSpace / static_cast<float>(visibleChildCount - 1);
}

void StackPanel::LayoutVertical() {
    const std::vector<Widget *> children = GetVisibleChildren(*this);

    if (children.empty()) {
        return;
    }

    const sf::Vector2f panelSize = GetSize();

    const float availableWidth = std::max(0.0f, panelSize.x - padding.left - padding.right);

    const float availableHeight = std::max(0.0f, panelSize.y - padding.top - padding.bottom);

    const float baseSpacing = spacing * static_cast<float>(children.size() - 1);

    const BoxConstraints childConstraints = ChildMeasureConstraints(StackOrientation::Vertical, availableWidth);

    float fixedHeight = 0.0f;
    float totalFlex = 0.0f;

    for (Widget *child : children) {
        const float flex = EffectiveFlex(*this, *child, StackOrientation::Vertical);
        const sf::Vector2f childDesiredSize = child->Measure(childConstraints);

        if (flex > 0.0f) {
            totalFlex += flex;
        } else {
            fixedHeight += childDesiredSize.y;
        }
    }

    const float flexibleHeight = std::max(0.0f, availableHeight - fixedHeight - baseSpacing);

    float unusedSpace = 0.0f;

    if (totalFlex <= 0.0f) {
        unusedSpace = flexibleHeight;
    }

    const float actualSpacing = CalculateSpacing(unusedSpace, children.size());

    float currentY = padding.top + CalculateLeadingOffset(unusedSpace, children.size());

    for (Widget *child : children) {
        const float flex = EffectiveFlex(*this, *child, StackOrientation::Vertical);
        const sf::Vector2f childDesiredSize = child->GetDesiredSize();

        const float childHeight =
            flex > 0.0f && totalFlex > 0.0f ? flexibleHeight * flex / totalFlex : childDesiredSize.y;

        float childWidth = childDesiredSize.x;
        float childX = padding.left;

        const bool stretchCrossAxis = crossAxisAlignment == CrossAxisAlignment::Stretch ||
                                      child->GetWidthPolicy() == SizePolicy::Stretch;

        switch (stretchCrossAxis ? CrossAxisAlignment::Stretch : crossAxisAlignment) {
        case CrossAxisAlignment::Stretch:
            childWidth = availableWidth;
            break;

        case CrossAxisAlignment::Center:
            childX += (availableWidth - childWidth) * 0.5f;
            break;

        case CrossAxisAlignment::End:
            childX += availableWidth - childWidth;
            break;

        case CrossAxisAlignment::Start:
            break;
        }

        child->Arrange({{childX, currentY}, {std::max(0.0f, childWidth), std::max(0.0f, childHeight)}});
        currentY += child->GetSize().y + actualSpacing;
    }
}

void StackPanel::LayoutHorizontal() {
    const std::vector<Widget *> children = GetVisibleChildren(*this);

    if (children.empty()) {
        return;
    }

    const sf::Vector2f panelSize = GetSize();

    const float availableWidth = std::max(0.0f, panelSize.x - padding.left - padding.right);

    const float availableHeight = std::max(0.0f, panelSize.y - padding.top - padding.bottom);

    const float baseSpacing = spacing * static_cast<float>(children.size() - 1);

    const BoxConstraints childConstraints = ChildMeasureConstraints(StackOrientation::Horizontal, availableHeight);

    float fixedWidth = 0.0f;
    float totalFlex = 0.0f;

    for (Widget *child : children) {
        const float flex = EffectiveFlex(*this, *child, StackOrientation::Horizontal);
        const sf::Vector2f childDesiredSize = child->Measure(childConstraints);

        if (flex > 0.0f) {
            totalFlex += flex;
        } else {
            fixedWidth += childDesiredSize.x;
        }
    }

    const float flexibleWidth = std::max(0.0f, availableWidth - fixedWidth - baseSpacing);

    float unusedSpace = 0.0f;

    if (totalFlex <= 0.0f) {
        unusedSpace = flexibleWidth;
    }

    const float actualSpacing = CalculateSpacing(unusedSpace, children.size());

    float currentX = padding.left + CalculateLeadingOffset(unusedSpace, children.size());

    for (Widget *child : children) {
        const float flex = EffectiveFlex(*this, *child, StackOrientation::Horizontal);
        const sf::Vector2f childDesiredSize = child->GetDesiredSize();

        const float childWidth =
            flex > 0.0f && totalFlex > 0.0f ? flexibleWidth * flex / totalFlex : childDesiredSize.x;

        float childHeight = childDesiredSize.y;
        float childY = padding.top;

        const bool stretchCrossAxis = crossAxisAlignment == CrossAxisAlignment::Stretch ||
                                      child->GetHeightPolicy() == SizePolicy::Stretch;

        switch (stretchCrossAxis ? CrossAxisAlignment::Stretch : crossAxisAlignment) {
        case CrossAxisAlignment::Stretch:
            childHeight = availableHeight;
            break;

        case CrossAxisAlignment::Center:
            childY += (availableHeight - childHeight) * 0.5f;
            break;

        case CrossAxisAlignment::End:
            childY += availableHeight - childHeight;
            break;

        case CrossAxisAlignment::Start:
            break;
        }

        child->Arrange({{currentX, childY}, {std::max(0.0f, childWidth), std::max(0.0f, childHeight)}});
        currentX += child->GetSize().x + actualSpacing;
    }
}

void StackPanel::OnChildRemoved(Widget &child) { childFlex.erase(&child); RefreshLayout(); }
