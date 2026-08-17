#include <PipeFrame/UI/Widget.h>

void Widget::SetPosition(sf::Vector2f newPosition) {
    position = newPosition;
    NotifyGeometryChanged();
}

void Widget::SetSize(sf::Vector2f newSize) {
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

void Widget::SetEnabled(bool newEnabled) { enabled = newEnabled; }

bool Widget::IsEnabled() const { return enabled; }

Widget *Widget::GetParent() { return parent; }

const Widget *Widget::GetParent() const { return parent; }

std::size_t Widget::GetChildCount() const { return children.size(); }

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

    childReference.NotifyGeometryChanged();
}

void Widget::NotifyGeometryChanged() {
    OnGeometryChanged();

    for (const std::unique_ptr<Widget> &child : children) {
        child->NotifyGeometryChanged();
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

    // Children created later are drawn later, so they are on top.
    for (auto iterator = children.rbegin(); iterator != children.rend(); ++iterator) {
        Widget *hitWidget = (*iterator)->FindTopmostAt(screenPoint);

        if (hitWidget != nullptr) {
            return hitWidget;
        }
    }

    if (Contains(screenPoint)) {
        return this;
    }

    return nullptr;
}