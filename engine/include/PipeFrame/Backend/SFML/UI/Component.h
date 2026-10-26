#ifndef PIPEFRAME_UI_COMPONENT_H
#define PIPEFRAME_UI_COMPONENT_H

#include <PipeFrame/Backend/SFML/UI/Widget.h>

#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pipeframe::ui {

template <typename WidgetType, typename ConfigureFunction, typename... ChildSpecs>
class ComponentSpec {
public:
    ComponentSpec(std::string newKey, ConfigureFunction newConfigure, ChildSpecs... newChildren)
        : key(std::move(newKey)), configure(std::move(newConfigure)), children(std::move(newChildren)...) {}

    WidgetType& Materialize(Widget& parent) {
        WidgetType& widget = parent.GetOrCreateChild<WidgetType>(key);
        widget.SetVisible(true);
        std::invoke(configure, widget);
        widget.BeginCompositionPass();
        try {
            std::apply([&widget](auto&... child) { (child.Materialize(widget), ...); }, children);
        } catch (...) {
            widget.EndCompositionPass();
            throw;
        }
        widget.EndCompositionPass();
        return widget;
    }

private:
    std::string key;
    ConfigureFunction configure;
    std::tuple<ChildSpecs...> children;
};

template <typename WidgetType, typename ConfigureFunction, typename... ChildSpecs>
auto Component(std::string key, ConfigureFunction&& configure, ChildSpecs&&... children) {
    static_assert(std::is_base_of_v<Widget, WidgetType>, "WidgetType must derive from Widget");
    return ComponentSpec<WidgetType, std::decay_t<ConfigureFunction>, std::decay_t<ChildSpecs>...>{
        std::move(key), std::forward<ConfigureFunction>(configure), std::forward<ChildSpecs>(children)...};
}

template <typename... ComponentSpecs>
void Compose(Widget& parent, ComponentSpecs&&... components) {
    parent.BeginCompositionPass();
    try {
        (components.Materialize(parent), ...);
    } catch (...) {
        parent.EndCompositionPass();
        throw;
    }
    parent.EndCompositionPass();
}

}  // namespace pipeframe::ui

#endif
