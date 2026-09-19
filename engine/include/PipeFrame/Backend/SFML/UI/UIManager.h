#ifndef PIPEFRAME_UI_MANAGER_H
#define PIPEFRAME_UI_MANAGER_H

#include <PipeFrame/Backend/SFML/UI/UITheme.h>
#include <PipeFrame/Backend/SFML/UI/Widget.h>
#include <PipeFrame/UI/MountedView.h>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

class UIManager {
public:
    UIManager() = default;

    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    template <typename WidgetType, typename... Arguments>
    WidgetType& CreateRoot(Arguments&&... arguments) {
        static_assert(std::is_base_of_v<Widget, WidgetType>, "WidgetType must derive from Widget");

        auto root = std::make_unique<WidgetType>(std::forward<Arguments>(arguments)...);

        WidgetType& rootReference = *root;

        (updating ? pendingRoots : roots).push_back(std::move(root));

        return rootReference;
    }

    pipeframe::ui::MountedView MountView(pipeframe::ui::View view, const UITheme& theme = UITheme::Dark());

    bool HandleEvent(const sf::Event& event);

    void Render(sf::RenderTarget& target) const;
    void Update(float realDeltaSeconds);

    bool LoadDefaultFont(const std::filesystem::path& fontPath);

    bool HasDefaultFont() const;

    const sf::Font& GetDefaultFont() const;

    std::size_t GetRootCount() const;
    Widget* GetRoot(std::size_t index) { return index < roots.size() ? roots[index].get() : nullptr; }

    bool HasKeyboardFocus() const;

private:
    static std::optional<sf::Vector2f> GetPointerPosition(const sf::Event& event);
    void UpdateHoveredWidget(Widget* newHoveredWidget);

    void DispatchEvent(Widget* target, const sf::Event& event);

    Widget* FindTopmostAt(sf::Vector2f screenPoint);

    sf::Font defaultFont;
    bool defaultFontLoaded = false;

    std::vector<std::unique_ptr<Widget>> roots, pendingRoots;
    bool updating{false};

    Widget* hoveredWidget = nullptr;
    Widget* capturedWidget = nullptr;
    Widget* focusedWidget = nullptr;
    std::weak_ptr<void> hoveredLifetime, capturedLifetime, focusedLifetime;
    void PruneExpiredInput();

    static Widget* FindFocusableAncestor(Widget* widget);
    static bool IsInteractiveInTree(const Widget* widget);
    void SetKeyboardFocus(Widget* widget);
};

#endif
