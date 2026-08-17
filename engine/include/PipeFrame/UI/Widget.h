#ifndef PIPEFRAME_WIDGET_H
#define PIPEFRAME_WIDGET_H

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <SFML/Window/Event.hpp>

class UIManager;

class Widget {
  public:
    Widget() = default;
    virtual ~Widget() = default;

    Widget(const Widget &) = delete;
    Widget &operator=(const Widget &) = delete;

    Widget(Widget &&) = delete;
    Widget &operator=(Widget &&) = delete;

    void SetPosition(sf::Vector2f newPosition);
    void SetSize(sf::Vector2f newSize);

    // Position relative to the parent.
    sf::Vector2f GetPosition() const;

    // Final position in screen coordinates.
    sf::Vector2f GetScreenPosition() const;

    sf::Vector2f GetSize() const;
    sf::FloatRect GetBounds() const;

    bool Contains(sf::Vector2f screenPoint) const;

    void SetVisible(bool newVisible);
    bool IsVisible() const;

    void SetEnabled(bool newEnabled);
    bool IsEnabled() const;

    Widget *GetParent();
    const Widget *GetParent() const;

    std::size_t GetChildCount() const;

    template <typename WidgetType, typename... Arguments> WidgetType &CreateChild(Arguments &&...arguments) {
        static_assert(std::is_base_of_v<Widget, WidgetType>, "WidgetType must derive from Widget");

        auto child = std::make_unique<WidgetType>(std::forward<Arguments>(arguments)...);

        WidgetType &childReference = *child;

        AttachChild(std::move(child));

        return childReference;
    }

    void Render(sf::RenderTarget &target) const;

  protected:
    virtual void OnRender(sf::RenderTarget &target) const = 0;
    virtual void OnGeometryChanged();
    virtual bool OnEvent(const sf::Event &event);
    
    virtual void OnPointerEntered();
    virtual void OnPointerExited();

  private:
    friend class UIManager;

    Widget *FindTopmostAt(sf::Vector2f screenPoint);
    void AttachChild(std::unique_ptr<Widget> child);
    void NotifyGeometryChanged();

    Widget *parent = nullptr;
    std::vector<std::unique_ptr<Widget>> children;

    sf::Vector2f position{0.0f, 0.0f};
    sf::Vector2f size{0.0f, 0.0f};

    bool visible = true;
    bool enabled = true;
};

#endif