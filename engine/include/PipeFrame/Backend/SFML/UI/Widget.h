#ifndef PIPEFRAME_WIDGET_H
#define PIPEFRAME_WIDGET_H

#include <PipeFrame/Backend/SFML/UI/Layout.h>
#include <PipeFrame/Backend/SFML/UI/Motion.h>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class UIManager;

class Widget {
public:
    Widget() = default;
    virtual ~Widget() = default;

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    Widget(Widget&&) = delete;
    Widget& operator=(Widget&&) = delete;

    void SetPosition(sf::Vector2f newPosition);
    void SetSize(sf::Vector2f newSize);

    void SetWidthPolicy(SizePolicy policy);
    void SetHeightPolicy(SizePolicy policy);
    void SetSizePolicy(SizePolicy widthPolicy, SizePolicy heightPolicy);

    SizePolicy GetWidthPolicy() const;
    SizePolicy GetHeightPolicy() const;

    void SetMinimumSize(sf::Vector2f minimumSize);
    void SetMaximumSize(sf::Vector2f maximumSize);

    sf::Vector2f GetMinimumSize() const;
    sf::Vector2f GetMaximumSize() const;
    sf::Vector2f GetRequestedSize() const;
    sf::Vector2f GetDesiredSize() const;

    sf::Vector2f Measure(const BoxConstraints& constraints);
    void Arrange(const sf::FloatRect& layoutBounds);

    sf::Vector2f GetPosition() const;
    sf::Vector2f GetScreenPosition() const;
    sf::Vector2f GetSize() const;

    void SetVisualOffset(sf::Vector2f offset);
    sf::Vector2f GetVisualOffset() const;
    void AnimateVisualOffsetTo(sf::Vector2f offset, float durationSeconds);

    void SetOpacity(float opacity);
    float GetOpacity() const;
    float GetEffectiveOpacity() const;
    void AnimateOpacityTo(float opacity, float durationSeconds);
    bool IsMotionActive() const;
    virtual void SetReducedMotion(bool reducedMotion);
    bool IsReducedMotion() const;

    sf::FloatRect GetBounds() const;

    bool Contains(sf::Vector2f screenPoint) const;

    void SetVisible(bool newVisible);
    bool IsVisible() const;

    void SetEnabled(bool newEnabled);
    bool IsEnabled() const;

    Widget* GetParent();
    const Widget* GetParent() const;

    void SetKey(std::string newKey);
    const std::string& GetKey() const;
    Widget* FindChildByKey(const std::string& key);
    const Widget* FindChildByKey(const std::string& key) const;

    void BeginCompositionPass(bool disposeUnused = false);
    void RemoveChild(Widget& child);
    const void* CompositionIdentity() const { return compositionIdentity; }
    void SetCompositionIdentity(const void* value) { compositionIdentity = value; }
    std::weak_ptr<void> Lifetime() const { return lifetime; }

    template <class T, class... Args>
    T& ReconcileChild(const std::string& key, Args&&... args) {
        if (auto* old = FindChildByKey(key); old && !dynamic_cast<T*>(old)) RemoveChild(*old);
        return GetOrCreateChild<T>(key, std::forward<Args>(args)...);
    }
    void EndCompositionPass();

    std::size_t GetChildCount() const;

    template <typename WidgetType, typename... Arguments>
    WidgetType& CreateChild(Arguments&&... arguments) {
        static_assert(std::is_base_of_v<Widget, WidgetType>, "WidgetType must derive from Widget");

        auto child = std::make_unique<WidgetType>(std::forward<Arguments>(arguments)...);

        WidgetType& childReference = *child;

        AttachChild(std::move(child));

        return childReference;
    }

    template <typename WidgetType, typename... Arguments>
    WidgetType& GetOrCreateChild(const std::string& key, Arguments&&... arguments) {
        static_assert(std::is_base_of_v<Widget, WidgetType>, "WidgetType must derive from Widget");

        if (compositionActive && !key.empty()) {
            composedKeys.push_back(key);
        }

        if (Widget* existing = FindChildByKey(key)) {
            if (auto* typed = dynamic_cast<WidgetType*>(existing)) {
                return *typed;
            }
            throw std::logic_error("A widget key cannot be reused with a different widget type");
        }

        WidgetType& child = CreateChild<WidgetType>(std::forward<Arguments>(arguments)...);
        child.SetKey(key);
        return child;
    }

    template <typename Handle>
    void RetainBinding(Handle&& handle) {
        retainedBindings.push_back(std::make_shared<std::decay_t<Handle>>(std::forward<Handle>(handle)));
    }

    virtual void Render(sf::RenderTarget& target) const;
    void Update(float realDeltaSeconds);
    virtual bool IsDisposed() const { return false; }

    void SetHitTestVisible(bool newHitTestVisible);

    bool IsHitTestVisible() const;

    Widget* GetChild(std::size_t index);
    const Widget* GetChild(std::size_t index) const;

    void SetFocusable(bool newFocusable);
    bool IsFocusable() const;
    bool HasKeyboardFocus() const;
    // Visible, enabled barriers constrain pointer capture and keyboard focus to their subtree.
    void SetInputBarrier(bool enabled) { inputBarrier = enabled; }
    bool IsInputBarrier() const { return inputBarrier; }

protected:
    virtual void OnRender(sf::RenderTarget& target) const = 0;

    virtual void OnGeometryChanged();
    virtual bool OnEvent(const sf::Event& event);

    virtual void OnPointerEntered();
    virtual void OnPointerExited();

    virtual void OnEnabledChanged();
    virtual void OnOpacityChanged();

    virtual void OnChildGeometryChanged(Widget& child);
    virtual void OnChildRemoved(Widget& child);

    virtual void OnKeyboardFocusGained();
    virtual void OnKeyboardFocusLost();

    virtual sf::Vector2f OnMeasure(const BoxConstraints& constraints);
    virtual void OnUpdate(float realDeltaSeconds);

    virtual bool ClipsChildren() const;

    void RenderChildren(sf::RenderTarget& target) const;

private:
    friend class UIManager;

public:
    // Hit-tests this subtree, respecting visibility and clipping within it.
    Widget* FindTopmostAt(sf::Vector2f screenPoint);

private:
    void AttachChild(std::unique_ptr<Widget> child);

    void NotifyGeometryChanged(bool notifyParent = true);
    void NotifyOpacityChanged();

    Widget* parent = nullptr;

    std::vector<std::unique_ptr<Widget>> children;
    std::vector<std::shared_ptr<void>> retainedBindings;

    std::string key;
    std::vector<std::string> composedKeys;
    bool compositionActive = false;
    bool disposeUncomposed = false;
    const void* compositionIdentity{};
    std::shared_ptr<void> lifetime = std::make_shared<int>(0);

    sf::Vector2f position{0.0f, 0.0f};
    sf::Vector2f visualOffset{0.0f, 0.0f};
    sf::Vector2f size{0.0f, 0.0f};
    sf::Vector2f requestedSize{0.0f, 0.0f};
    sf::Vector2f desiredSize{0.0f, 0.0f};
    sf::Vector2f minimumSize{0.0f, 0.0f};
    sf::Vector2f maximumSize{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};

    SizePolicy widthPolicy = SizePolicy::Fixed;
    SizePolicy heightPolicy = SizePolicy::Fixed;

    bool visible = true;
    bool enabled = true;
    bool hitTestVisible = true;

    bool focusable = false;
    bool keyboardFocused = false;
    bool inputBarrier = false;

    pipeframe::ui::AnimatedVector2 animatedVisualOffset;
    pipeframe::ui::AnimatedFloat animatedOpacity{1.0f};
    float visualOffsetDuration = 0.0f;
    float opacityDuration = 0.0f;
    float opacity = 1.0f;
    bool reducedMotion = false;
};

#endif
