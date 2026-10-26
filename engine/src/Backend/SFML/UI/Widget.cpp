#include <PipeFrame/Backend/SFML/UI/Widget.h>

#include <algorithm>

namespace {

sf::Vector2f SanitizeMinimum(const sf::Vector2f size) { return {std::max(0.0f, size.x), std::max(0.0f, size.y)}; }

sf::Vector2f SanitizeMaximum(const sf::Vector2f size, const sf::Vector2f minimum) {
    return {std::max(minimum.x, size.x), std::max(minimum.y, size.y)};
}

BoxConstraints IntersectConstraints(const BoxConstraints &parent, const sf::Vector2f minimum,
                                    const sf::Vector2f maximum) {
    BoxConstraints result;
    result.minimum = {std::max(parent.minimum.x, minimum.x), std::max(parent.minimum.y, minimum.y)};
    result.maximum = {std::min(parent.maximum.x, maximum.x), std::min(parent.maximum.y, maximum.y)};
    result.maximum.x = std::max(result.minimum.x, result.maximum.x);
    result.maximum.y = std::max(result.minimum.y, result.maximum.y);
    return result;
}

} // namespace

void Widget::SetPosition(const sf::Vector2f newPosition) {

    if (position == newPosition) {
        return;
    }

    position = newPosition;
    NotifyGeometryChanged();
}

void Widget::SetSize(const sf::Vector2f newSize) {
    requestedSize = SanitizeMinimum(newSize);
    const sf::Vector2f constrainedSize = BoxConstraints{minimumSize, maximumSize}.Constrain(requestedSize);

    if (size == constrainedSize) {
        return;
    }

    size = constrainedSize;
    NotifyGeometryChanged();
}

void Widget::SetWidthPolicy(const SizePolicy policy) {
    if (widthPolicy == policy) {
        return;
    }

    widthPolicy = policy;
    NotifyGeometryChanged();
}

void Widget::SetHeightPolicy(const SizePolicy policy) {
    if (heightPolicy == policy) {
        return;
    }

    heightPolicy = policy;
    NotifyGeometryChanged();
}

void Widget::SetSizePolicy(const SizePolicy newWidthPolicy, const SizePolicy newHeightPolicy) {
    const bool changed = widthPolicy != newWidthPolicy || heightPolicy != newHeightPolicy;
    widthPolicy = newWidthPolicy;
    heightPolicy = newHeightPolicy;

    if (changed) {
        NotifyGeometryChanged();
    }
}

SizePolicy Widget::GetWidthPolicy() const { return widthPolicy; }

SizePolicy Widget::GetHeightPolicy() const { return heightPolicy; }

void Widget::SetMinimumSize(const sf::Vector2f newMinimumSize) {
    const sf::Vector2f sanitized = SanitizeMinimum(newMinimumSize);
    if (minimumSize == sanitized) {
        return;
    }

    minimumSize = sanitized;
    maximumSize = SanitizeMaximum(maximumSize, minimumSize);
    size = BoxConstraints{minimumSize, maximumSize}.Constrain(size);
    NotifyGeometryChanged();
}

void Widget::SetMaximumSize(const sf::Vector2f newMaximumSize) {
    const sf::Vector2f sanitized = SanitizeMaximum(newMaximumSize, minimumSize);
    if (maximumSize == sanitized) {
        return;
    }

    maximumSize = sanitized;
    size = BoxConstraints{minimumSize, maximumSize}.Constrain(size);
    NotifyGeometryChanged();
}

sf::Vector2f Widget::GetMinimumSize() const { return minimumSize; }

sf::Vector2f Widget::GetMaximumSize() const { return maximumSize; }

sf::Vector2f Widget::GetRequestedSize() const { return requestedSize; }

sf::Vector2f Widget::GetDesiredSize() const { return desiredSize; }

sf::Vector2f Widget::Measure(const BoxConstraints &constraints) {
    const BoxConstraints effective = IntersectConstraints(constraints, minimumSize, maximumSize);
    const sf::Vector2f contentSize = OnMeasure(effective);

    sf::Vector2f result = contentSize;

    if (widthPolicy == SizePolicy::Fixed) {
        result.x = requestedSize.x;
    } else if (widthPolicy == SizePolicy::Stretch && effective.HasBoundedWidth()) {
        result.x = effective.maximum.x;
    }

    if (heightPolicy == SizePolicy::Fixed) {
        result.y = requestedSize.y;
    } else if (heightPolicy == SizePolicy::Stretch && effective.HasBoundedHeight()) {
        result.y = effective.maximum.y;
    }

    desiredSize = effective.Constrain(result);
    return desiredSize;
}

void Widget::Arrange(const sf::FloatRect &layoutBounds) {
    const sf::Vector2f arrangedSize = BoxConstraints{minimumSize, maximumSize}.Constrain(layoutBounds.size);

    if (position == layoutBounds.position && size == arrangedSize) {
        return;
    }

    position = layoutBounds.position;
    size = arrangedSize;
    NotifyGeometryChanged();
}

sf::Vector2f Widget::GetPosition() const { return position; }

sf::Vector2f Widget::GetScreenPosition() const {
    if (parent == nullptr) {
        return position + visualOffset;
    }

    return parent->GetScreenPosition() + position + visualOffset;
}

sf::Vector2f Widget::GetSize() const { return size; }

void Widget::SetVisualOffset(const sf::Vector2f offset) {
    animatedVisualOffset.SetTarget(offset, true);
    if (visualOffset == offset) {
        return;
    }
    visualOffset = offset;
    NotifyGeometryChanged(false);
}

sf::Vector2f Widget::GetVisualOffset() const { return visualOffset; }

void Widget::AnimateVisualOffsetTo(const sf::Vector2f offset, const float durationSeconds) {
    visualOffsetDuration = std::max(0.0f, durationSeconds);
    animatedVisualOffset.SetTarget(offset, reducedMotion);
    if (reducedMotion) {
        visualOffset = offset;
        NotifyGeometryChanged(false);
    }
}

void Widget::SetOpacity(const float newOpacity) {
    const float clamped = std::clamp(newOpacity, 0.0f, 1.0f);
    animatedOpacity.SetTarget(clamped, true);
    if (opacity == clamped) {
        return;
    }
    opacity = clamped;
    NotifyOpacityChanged();
}

float Widget::GetOpacity() const { return opacity; }

float Widget::GetEffectiveOpacity() const {
    return opacity * (parent == nullptr ? 1.0f : parent->GetEffectiveOpacity());
}

void Widget::AnimateOpacityTo(const float newOpacity, const float durationSeconds) {
    opacityDuration = std::max(0.0f, durationSeconds);
    animatedOpacity.SetTarget(std::clamp(newOpacity, 0.0f, 1.0f), reducedMotion);
    if (reducedMotion) {
        opacity = animatedOpacity.Get();
        NotifyOpacityChanged();
    }
}

bool Widget::IsMotionActive() const {
    return animatedOpacity.Get() != animatedOpacity.GetTarget() ||
           animatedVisualOffset.Get() != animatedVisualOffset.GetTarget();
}

void Widget::SetReducedMotion(const bool newReducedMotion) {
    reducedMotion = newReducedMotion;
    if (reducedMotion) {
        animatedOpacity.SetTarget(animatedOpacity.GetTarget(), true);
        animatedVisualOffset.SetTarget(animatedVisualOffset.GetTarget(), true);
        opacity = animatedOpacity.Get();
        visualOffset = animatedVisualOffset.Get();
        NotifyOpacityChanged();
        NotifyGeometryChanged(false);
    }
    for (const std::unique_ptr<Widget> &child : children) {
        child->SetReducedMotion(newReducedMotion);
    }
}

bool Widget::IsReducedMotion() const { return reducedMotion; }

sf::FloatRect Widget::GetBounds() const {
    return {
        GetScreenPosition(),
        size,
    };
}

bool Widget::Contains(const sf::Vector2f screenPoint) const { return GetBounds().contains(screenPoint); }

void Widget::SetVisible(const bool newVisible) {

    if (visible == newVisible) {
        return;
    }

    visible = newVisible;

    if (parent != nullptr) {
        parent->OnChildGeometryChanged(*this);
    }
}

bool Widget::IsVisible() const { return visible; }

void Widget::SetEnabled(const bool newEnabled) {

    if (enabled == newEnabled) {
        return;
    }

    enabled = newEnabled;
    OnEnabledChanged();
}

bool Widget::IsEnabled() const { return enabled; }

Widget *Widget::GetParent() { return parent; }

const Widget *Widget::GetParent() const { return parent; }

void Widget::SetKey(std::string newKey) {
    if (parent != nullptr && !newKey.empty()) {
        if (Widget *existing = parent->FindChildByKey(newKey); existing != nullptr && existing != this) {
            throw std::logic_error("Sibling widgets must have unique non-empty keys");
        }
    }
    key = std::move(newKey);
}

const std::string &Widget::GetKey() const { return key; }

Widget *Widget::FindChildByKey(const std::string &childKey) {
    if (childKey.empty()) {
        return nullptr;
    }
    for (const std::unique_ptr<Widget> &child : children) {
        if (child->key == childKey) {
            return child.get();
        }
    }
    return nullptr;
}

const Widget *Widget::FindChildByKey(const std::string &childKey) const {
    return const_cast<Widget *>(this)->FindChildByKey(childKey);
}

void Widget::BeginCompositionPass(bool disposeUnused) {
    disposeUncomposed = disposeUnused;
    compositionActive = true;
    composedKeys.clear();
}

void Widget::EndCompositionPass() {
    if (!compositionActive) {
        return;
    }

    compositionActive = false;
    if (disposeUncomposed) {
        for (std::size_t i = children.size(); i > 0; --i) {
            auto &child = *children[i - 1];
            if (!child.key.empty() &&
                std::find(composedKeys.begin(), composedKeys.end(), child.key) == composedKeys.end())
                RemoveChild(child);
        }
        // Description order controls both layout and hit testing; identity is retained by key.
        std::stable_sort(children.begin(), children.end(), [&](const auto &a, const auto &b) {
            return std::find(composedKeys.begin(), composedKeys.end(), a->key) <
                   std::find(composedKeys.begin(), composedKeys.end(), b->key);
        });
        if (!children.empty())
            OnChildGeometryChanged(*children.front());
    } else {
        for (const auto &child : children) {
            if (!child->key.empty())
                child->SetVisible(std::find(composedKeys.begin(), composedKeys.end(), child->key) !=
                                  composedKeys.end());
        }
    }
    composedKeys.clear();
}

std::size_t Widget::GetChildCount() const { return children.size(); }

Widget *Widget::GetChild(const std::size_t index) {

    if (index >= children.size()) {
        return nullptr;
    }

    return children[index].get();
}

const Widget *Widget::GetChild(const std::size_t index) const {

    if (index >= children.size()) {
        return nullptr;
    }

    return children[index].get();
}

void Widget::Render(sf::RenderTarget &target) const {

    if (!visible) {
        return;
    }

    OnRender(target);
    RenderChildren(target);
}

void Widget::Update(const float realDeltaSeconds) {
    if (!visible) {
        return;
    }
    const float delta = std::max(0.0f, realDeltaSeconds);
    if (animatedVisualOffset.Update(delta, reducedMotion ? 0.0f : visualOffsetDuration)) {
        visualOffset = animatedVisualOffset.Get();
        NotifyGeometryChanged(false);
    }
    if (animatedOpacity.Update(delta, reducedMotion ? 0.0f : opacityDuration)) {
        opacity = animatedOpacity.Get();
        NotifyOpacityChanged();
    }
    OnUpdate(delta);
    for (const std::unique_ptr<Widget> &child : children) {
        child->Update(realDeltaSeconds);
    }
    for (std::size_t i = children.size(); i > 0; --i)
        if (children[i - 1]->IsDisposed())
            RemoveChild(*children[i - 1]);
}

void Widget::RenderChildren(sf::RenderTarget &target) const {
    const auto previous = target.getView();
    if (ClipsChildren()) {
        const auto size = target.getSize();
        if (!size.x || !size.y)
            return;
        const auto start = target.mapCoordsToPixel(GetScreenPosition());
        const auto end = target.mapCoordsToPixel(GetScreenPosition() + GetSize());
        const sf::FloatRect clip{
            {float(start.x) / size.x, float(start.y) / size.y},
            {float(std::max(0, end.x - start.x)) / size.x, float(std::max(0, end.y - start.y)) / size.y}};
        auto view = previous;
        view.setScissor(previous.getScissor().findIntersection(clip).value_or(sf::FloatRect{}));
        target.setView(view);
    }
    for (const auto &child : children)
        child->Render(target);
    if (ClipsChildren())
        target.setView(previous);
}

void Widget::OnGeometryChanged() {}

void Widget::OnChildGeometryChanged(Widget &child) { (void)child; }

void Widget::AttachChild(std::unique_ptr<Widget> child) {

    if (child == nullptr) {
        return;
    }

    child->parent = this;

    Widget &childReference = *child;

    children.push_back(std::move(child));

    childReference.NotifyGeometryChanged(false);
    childReference.NotifyOpacityChanged();

    OnChildGeometryChanged(childReference);
}

void Widget::NotifyGeometryChanged(const bool notifyParent) {

    OnGeometryChanged();

    if (notifyParent && parent != nullptr) {
        parent->OnChildGeometryChanged(*this);
    }

    for (const std::unique_ptr<Widget> &child : children) {
        child->NotifyGeometryChanged(false);
    }
}

void Widget::NotifyOpacityChanged() {
    OnOpacityChanged();
    for (const std::unique_ptr<Widget> &child : children) {
        child->NotifyOpacityChanged();
    }
}

bool Widget::OnEvent(const sf::Event &event) {

    (void)event;
    return false;
}

void Widget::OnPointerEntered() {}

void Widget::OnPointerExited() {}

void Widget::OnEnabledChanged() {}

void Widget::OnOpacityChanged() {}

void Widget::SetHitTestVisible(const bool newHitTestVisible) { hitTestVisible = newHitTestVisible; }

bool Widget::IsHitTestVisible() const { return hitTestVisible; }

void Widget::SetFocusable(const bool newFocusable) { focusable = newFocusable; }

bool Widget::IsFocusable() const { return focusable; }

bool Widget::HasKeyboardFocus() const { return keyboardFocused; }

void Widget::OnKeyboardFocusGained() {}

void Widget::OnKeyboardFocusLost() {}

sf::Vector2f Widget::OnMeasure(const BoxConstraints &constraints) { return constraints.Constrain(requestedSize); }

void Widget::OnUpdate(const float realDeltaSeconds) { (void)realDeltaSeconds; }

bool Widget::ClipsChildren() const { return false; }

Widget *Widget::FindTopmostAt(const sf::Vector2f screenPoint) {

    if (!visible || !enabled) {
        return nullptr;
    }

    if (ClipsChildren() && !Contains(screenPoint)) {
        return nullptr;
    }

    for (auto iterator = children.rbegin(); iterator != children.rend(); ++iterator) {

        Widget *hitWidget = (*iterator)->FindTopmostAt(screenPoint);

        if (hitWidget != nullptr) {
            return hitWidget;
        }
    }

    if (hitTestVisible && Contains(screenPoint)) {
        return this;
    }

    return nullptr;
}

void Widget::RemoveChild(Widget &child) {
    const auto it =
        std::find_if(children.begin(), children.end(), [&](const auto &item) { return item.get() == &child; });
    if (it == children.end())
        return;
    auto removed = std::move(*it);
    children.erase(it);
    removed->SetVisible(false);
    OnChildRemoved(*removed);
}
void Widget::OnChildRemoved(Widget &) {}
