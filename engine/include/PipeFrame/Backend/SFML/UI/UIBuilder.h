#ifndef PIPEFRAME_UI_BUILDER_H
#define PIPEFRAME_UI_BUILDER_H

#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <PipeFrame/Backend/SFML/UI/Widget.h>

#include <functional>
#include <type_traits>
#include <utility>

namespace pipeframe::ui {

template <typename WidgetType, typename ConfigureFunction, typename... Arguments>
WidgetType& BuildChild(Widget& parent, ConfigureFunction&& configure, Arguments&&... arguments) {
    static_assert(std::is_base_of_v<Widget, WidgetType>, "WidgetType must derive from Widget");

    WidgetType& widget = parent.CreateChild<WidgetType>(std::forward<Arguments>(arguments)...);

    std::invoke(std::forward<ConfigureFunction>(configure), widget);

    return widget;
}

template <typename WidgetType, typename ConfigureFunction, typename... Arguments>
WidgetType& BuildKeyedChild(Widget& parent, const std::string& key, ConfigureFunction&& configure,
                            Arguments&&... arguments) {
    static_assert(std::is_base_of_v<Widget, WidgetType>, "WidgetType must derive from Widget");

    WidgetType& widget = parent.GetOrCreateChild<WidgetType>(key, std::forward<Arguments>(arguments)...);
    widget.SetVisible(true);
    std::invoke(std::forward<ConfigureFunction>(configure), widget);
    return widget;
}

template <typename WidgetType, typename ConfigureFunction, typename... Arguments>
WidgetType& BuildRoot(UIManager& manager, ConfigureFunction&& configure, Arguments&&... arguments) {
    static_assert(std::is_base_of_v<Widget, WidgetType>, "WidgetType must derive from Widget");

    WidgetType& widget = manager.CreateRoot<WidgetType>(std::forward<Arguments>(arguments)...);

    std::invoke(std::forward<ConfigureFunction>(configure), widget);

    return widget;
}

template <typename WidgetType, typename ConfigureFunction>
WidgetType& Configure(WidgetType& widget, ConfigureFunction&& configure) {
    std::invoke(std::forward<ConfigureFunction>(configure), widget);

    return widget;
}

}  // namespace pipeframe::ui

#endif
