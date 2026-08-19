#include <PipeFrame/UI/Widget.h>

void Widget::SetPosition(sf::Vector2f newPosition) {
    if (position.x == newPosition.x && position.y == newPosition.y) {
        return;
    }

    position = newPosition;
    NotifyGeometryChanged();
}

void Widget::SetSize(sf::Vector2f newSize) {
    if (size.x == newSize.x && size.y == newSize.y) {
        return;
    }

    size = newSize;
    NotifyGeometryChanged();
}

sf::Vector2f Widget::GetPosition() const { return position; }

sf::Vector2f Widget::GetScreenPosition() const {
    if (parent == nullptr) {
        return position;
    }

    return parent->GetScreenPosition() + position;
}

sf::Vector2f Widget::GetSize() const { return size; }

sf::FloatRect Widget::GetBounds() const { return {GetScreenPosition(), size}; }

bool Widget::Contains(sf::Vector2f screenPoint) const { return GetBounds().contains(screenPoint); }

void Widget::SetVisible(bool newVisible) { visible = newVisible; }

bool Widget::IsVisible() const { return visible; }

void Widget::SetEnabled(bool newEnabled) {
    if (enabled == newEnabled) {
        return;
    }

    enabled = newEnabled;
    OnEnabledChanged();
}

void Widget::OnEnabledChanged() {}

bool Widget::IsEnabled() const { return enabled; }

Widget *Widget::GetParent() { return parent; }

const Widget *Widget::GetParent() const { return parent; }

std::size_t Widget::GetChildCount() const { return children.size(); }

Widget *Widget::GetChild(std::size_t index) {
    if (index >= children.size()) {
        return nullptr;
    }

    return children[index].get();
}

const Widget *Widget::GetChild(std::size_t index) const {
    if (index >= children.size()) {
        return nullptr;
    }

    return children[index].get();
}

void Widget::OnChildGeometryChanged(Widget &child) { (void)child; }

void Widget::Render(sf::RenderTarget &target) const {
    if (!visible) {
        return;
    }

    OnRender(target);

    for (const std::unique_ptr<Widget> &child : children) {
        child->Render(target);
    }
}

void Widget::OnGeometryChanged() {}

void Widget::AttachChild(std::unique_ptr<Widget> child) {
    if (child == nullptr) {
        return;
    }

    child->parent = this;

    Widget &childReference = *child;

    children.push_back(std::move(child));

    childReference.NotifyGeometryChanged(false);

    OnChildGeometryChanged(childReference);
}

void Widget::NotifyGeometryChanged(bool notifyParent) {
    OnGeometryChanged();

    if (notifyParent && parent != nullptr) {
        parent->OnChildGeometryChanged(*this);
    }

    // A parent movement changes every descendant's screen position.
    // Do not notify the parent again during this downward propagation.
    for (const std::unique_ptr<Widget> &child : children) {
        child->NotifyGeometryChanged(false);
    }
}

bool Widget::OnEvent(const sf::Event &event) {
    (void)event;
    return false;
}

void Widget::OnPointerEntered() {}

void Widget::OnPointerExited() {}

Widget *Widget::FindTopmostAt(sf::Vector2f screenPoint) {
    if (!visible || !enabled) {
        return nullptr;
    }

    // Search children first because they render above their parent.
    // Search in reverse because the last-created child is on top.
    for (auto iterator = children.rbegin(); iterator != children.rend(); ++iterator) {
        Widget *hitWidget = (*iterator)->FindTopmostAt(screenPoint);

        if (hitWidget != nullptr) {
            return hitWidget;
        }
    }

    // Only return this widget if it participates in pointer input.
    if (hitTestVisible && Contains(screenPoint)) {
        return this;
    }

    return nullptr;
}

void Widget::SetHitTestVisible(bool newHitTestVisible) { hitTestVisible = newHitTestVisible; }

bool Widget::IsHitTestVisible() const { return hitTestVisible; }