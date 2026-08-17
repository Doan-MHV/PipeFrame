#ifndef PIPEFRAME_UI_MANAGER_H
#define PIPEFRAME_UI_MANAGER_H

#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>

#include <PipeFrame/UI/Widget.h>

class UIManager {
  public:
    UIManager() = default;

    UIManager(const UIManager &) = delete;
    UIManager &operator=(const UIManager &) = delete;

    template <typename WidgetType, typename... Arguments> WidgetType &CreateRoot(Arguments &&...arguments) {
        static_assert(std::is_base_of_v<Widget, WidgetType>, "WidgetType must derive from Widget");

        auto root = std::make_unique<WidgetType>(std::forward<Arguments>(arguments)...);

        WidgetType &rootReference = *root;

        roots.push_back(std::move(root));

        return rootReference;
    }

    bool HandleEvent(const sf::Event &event);

    void Render(sf::RenderTarget &target) const;

    std::size_t GetRootCount() const;

  private:
    static std::optional<sf::Vector2f> GetPointerPosition(const sf::Event &event);
    void UpdateHoveredWidget(Widget *newHoveredWidget);

    void DispatchEvent(Widget *target, const sf::Event &event);

    Widget *FindTopmostAt(sf::Vector2f screenPoint);

    std::vector<std::unique_ptr<Widget>> roots;
    
    Widget *hoveredWidget = nullptr;
    Widget *capturedWidget = nullptr;
};

#endif