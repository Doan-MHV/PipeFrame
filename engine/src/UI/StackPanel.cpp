#include <PipeFrame/UI/StackPanel.h>

#include <algorithm>

void StackPanel::SetOrientation(StackOrientation newOrientation) {
    if (orientation == newOrientation) {
        return;
    }

    orientation = newOrientation;
    RefreshLayout();
}

StackOrientation StackPanel::GetOrientation() const { return orientation; }

void StackPanel::SetPadding(Thickness newPadding) {
    padding = newPadding;
    RefreshLayout();
}

Thickness StackPanel::GetPadding() const { return padding; }

void StackPanel::SetSpacing(float newSpacing) {
    spacing = std::max(0.0f, newSpacing);
    RefreshLayout();
}

float StackPanel::GetSpacing() const { return spacing; }

void StackPanel::RefreshLayout() {
    if (layoutInProgress) {
        return;
    }

    layoutInProgress = true;

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

void StackPanel::LayoutVertical() {
    const sf::Vector2f panelSize = GetSize();

    const float availableWidth = std::max(0.0f, panelSize.x - padding.left - padding.right);

    float currentY = padding.top;

    for (std::size_t index = 0; index < GetChildCount(); ++index) {
        Widget *child = GetChild(index);

        if (child == nullptr || !child->IsVisible()) {
            continue;
        }

        const float childHeight = child->GetSize().y;

        child->SetPosition({padding.left, currentY});

        child->SetSize({availableWidth, childHeight});

        currentY += childHeight + spacing;
    }
}

void StackPanel::LayoutHorizontal() {
    const sf::Vector2f panelSize = GetSize();

    const float availableHeight = std::max(0.0f, panelSize.y - padding.top - padding.bottom);

    float currentX = padding.left;

    for (std::size_t index = 0; index < GetChildCount(); ++index) {
        Widget *child = GetChild(index);

        if (child == nullptr || !child->IsVisible()) {
            continue;
        }

        const float childWidth = child->GetSize().x;

        child->SetPosition({currentX, padding.top});

        child->SetSize({childWidth, availableHeight});

        currentX += childWidth + spacing;
    }
}
