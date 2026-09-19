#ifndef PIPEFRAME_UI_VIEW_H
#define PIPEFRAME_UI_VIEW_H
#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/Render/RenderTypes.h>
#include <PipeFrame/Resources/GraphicsResourceService.h>

#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pipeframe::ui {
class ViewSource;
// Backend-neutral descriptions. A view never owns a window, font or widget.
struct View {
    enum class Kind {
        Row,
        Column,
        Text,
        Button,
        Input,
        Slider,
        Scroll,
        Stack,
        Popup,
        Stateful,
        Progress,
        Chart,
        NumberField,
        TextField,
        Mesh,
        Wrap
    };
    Kind kind;
    std::string key, text;
    std::function<void()> onPressed;
    std::vector<View> children;
    bool enabled{true};
    bool selected{false}, surface{false}, leading{false};
    View& Leading(bool value = true) {
        leading = value;
        return *this;
    }
    struct Series {
        std::vector<Vector2f> samples;
        Color color{100, 180, 255};
    };
    std::vector<Series> series;
    struct MeshLayer {
        std::vector<Vertex2D> vertices;
        TextureHandle texture;
    };
    std::vector<MeshLayer> mesh;
    std::shared_ptr<GraphicsResourceService> resources;
    View& Selected(bool value = true) {
        selected = value;
        return *this;
    }
    View& Surface(bool value = true) {
        surface = value;
        return *this;
    }
    float spacing{6};
    float height{34};
    float width{0}, padding{0}, flex{0};
    bool fitHeight{false}, stretchHeight{false};
    float minimum{0}, maximum{1}, value{0}, step{0};
    std::string inputValue;
    std::function<void(const std::string&)> onCommitted;
    std::function<void(float)> onChanged;
    std::shared_ptr<ViewSource> source;
    const void* instanceIdentity{};  // Internal mounted boundary identity.
    View& Height(float value) {
        height = value;
        fitHeight = false;
        stretchHeight = false;
        return *this;
    }
    View& Width(float value) {
        width = value;
        return *this;
    }
    View& Padding(float value) {
        padding = value;
        return *this;
    }
    View& Spacing(float value) {
        spacing = value;
        return *this;
    }
    View& Expanded(float weight = 1) {
        flex = weight;
        return *this;
    }
    View& FitHeight() {
        fitHeight = true;
        return *this;
    }
    View& FillHeight() {
        stretchHeight = true;
        return *this;
    }
    View& Enabled(bool value) {
        enabled = value;
        return *this;
    }
};
// Validate a complete description before touching retained widgets.
inline void ValidateView(const View& view) {
    if (view.key.empty()) throw std::invalid_argument("Views require stable keys");
    if (!std::isfinite(view.height) || view.height < 0 || !std::isfinite(view.spacing) || view.spacing < 0 ||
        !std::isfinite(view.width) || view.width < 0 || !std::isfinite(view.padding) || view.padding < 0 ||
        !std::isfinite(view.flex) || view.flex < 0)
        throw std::invalid_argument("View dimensions must be finite and nonnegative");
    const bool container = view.kind == View::Kind::Row || view.kind == View::Kind::Column ||
                           view.kind == View::Kind::Wrap || view.kind == View::Kind::Scroll ||
                           view.kind == View::Kind::Stack || view.kind == View::Kind::Popup;
    if (view.kind == View::Kind::Popup && view.children.size() != 1)
        throw std::invalid_argument("Popup needs one content view; compose its controls with Row/Column");
    if (view.kind == View::Kind::Scroll && view.children.size() != 1)
        throw std::invalid_argument("Scroll needs one content view");
    if (view.kind == View::Kind::Stateful && !view.source) throw std::invalid_argument("Stateful view needs a source");
    if (view.kind == View::Kind::Wrap && (!std::isfinite(view.minimum) || view.minimum <= 0))
        throw std::invalid_argument("Wrap requires a positive minimum item width");
    if ((view.kind == View::Kind::Progress || view.kind == View::Kind::NumberField) && !std::isfinite(view.value))
        throw std::invalid_argument("UI value must be finite");
    if (view.kind == View::Kind::Slider &&
        (!std::isfinite(view.minimum) || !std::isfinite(view.maximum) || !std::isfinite(view.value) ||
         !std::isfinite(view.step) || view.step < 0 || view.maximum < view.minimum))
        throw std::invalid_argument("Invalid slider range or value");
    if (!container && !view.children.empty()) throw std::invalid_argument("Leaf views cannot contain children");
    std::unordered_set<std::string> keys;
    for (const auto& child : view.children) {
        if (!keys.insert(child.key).second) throw std::invalid_argument("Duplicate sibling view key: " + child.key);
        ValidateView(child);
    }
}
namespace views {
inline View Button(std::string key, std::string text, std::function<void()> onPressed) {
    View result{View::Kind::Button, std::move(key), std::move(text), std::move(onPressed)};
    result.fitHeight = true;
    return result;
}
inline View Toggle(std::string key, std::string label, bool value, std::function<void(bool)> onChanged) {
    return Button(std::move(key), std::move(label) + (value ? " ON" : " OFF"),
                  [value, onChanged = std::move(onChanged)] {
                      if (onChanged) onChanged(!value);
                  });
}
inline View Text(std::string key, std::string text) {
    return {View::Kind::Text, std::move(key), std::move(text)};
}
inline View Mesh(std::string key, std::vector<View::MeshLayer> layers,
                 std::shared_ptr<GraphicsResourceService> resources) {
    View result{View::Kind::Mesh, std::move(key)};
    result.mesh = std::move(layers);
    result.resources = std::move(resources);
    result.height = 100;
    return result;
}
inline View Progress(std::string key, float value) {
    View result{View::Kind::Progress, std::move(key)};
    result.value = value;
    result.height = 10;
    return result;
}
inline View Chart(std::string key, std::vector<View::Series> series) {
    View result{View::Kind::Chart, std::move(key)};
    result.series = std::move(series);
    result.height = 90;
    return result;
}
inline View Card(std::string key, std::string title, std::string value, std::string detail = {});
inline View Row(std::string key, std::vector<View> children) {
    View result{View::Kind::Row, std::move(key), {}, {}, std::move(children)};
    result.fitHeight = true;
    return result;
}
inline View Wrap(std::string key, std::vector<View> children, float minimumWidth = 100) {
    View result{View::Kind::Wrap, std::move(key), {}, {}, std::move(children)};
    result.minimum = minimumWidth;
    result.fitHeight = true;
    return result;
}
inline View Column(std::string key, std::vector<View> children) {
    View result{View::Kind::Column, std::move(key), {}, {}, std::move(children)};
    result.fitHeight = true;
    return result;
}
inline View Card(std::string key, std::string title, std::string value, std::string detail) {
    return Column(std::move(key),
                  {Text("title", std::move(title)).FitHeight(), Text("value", std::move(value)).FitHeight(),
                   Text("detail", std::move(detail)).FitHeight()})
        .Padding(10)
        .Surface();
}
inline View Input(std::string key, std::string value, std::function<void(const std::string&)> commit) {
    View result{View::Kind::Input, std::move(key), std::move(value)};
    result.onCommitted = std::move(commit);
    return result;
}
inline View NumberField(std::string key, std::string label, float value, std::function<void(float)> commit) {
    View result{View::Kind::NumberField, std::move(key), std::move(label)};
    result.value = value;
    result.onChanged = std::move(commit);
    result.fitHeight = true;
    return result;
}
inline View TextField(std::string key, std::string label, std::string value,
                      std::function<void(const std::string&)> commit) {
    View result{View::Kind::TextField, std::move(key), std::move(label)};
    result.inputValue = std::move(value);
    result.onCommitted = std::move(commit);
    result.fitHeight = true;
    return result;
}
inline View Slider(std::string key, float value, float minimum, float maximum, std::function<void(float)> change) {
    View result{View::Kind::Slider, std::move(key)};
    result.value = value;
    result.minimum = minimum;
    result.maximum = maximum;
    result.onChanged = std::move(change);
    return result;
}
inline View Scroll(std::string key, View content) {
    return {View::Kind::Scroll, std::move(key), {}, {}, {std::move(content)}};
}
inline View Stack(std::string key, std::vector<View> children) {
    return {View::Kind::Stack, std::move(key), {}, {}, std::move(children)};
}
// Popup is a modal overlay within its host bounds; omit it from the tree to close.
inline View Popup(std::string key, std::vector<View> children) {
    return {View::Kind::Popup, std::move(key), {}, {}, std::move(children)};
}
inline View Stateful(std::string key, std::shared_ptr<ViewSource> source) {
    View result{View::Kind::Stateful, std::move(key)};
    result.source = std::move(source);
    return result;
}

}  // namespace views
}  // namespace pipeframe::ui
#endif
