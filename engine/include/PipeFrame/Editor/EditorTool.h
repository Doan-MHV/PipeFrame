#ifndef PIPEFRAME_EDITOR_TOOL_H
#define PIPEFRAME_EDITOR_TOOL_H

#include <string_view>

namespace pipeframe {

class EditorTool {
public:
    virtual ~EditorTool() = default;
    [[nodiscard]] virtual std::string_view GetToolId() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
    [[nodiscard]] virtual bool IsEnabled() const = 0;
};

}  // namespace pipeframe

#endif
