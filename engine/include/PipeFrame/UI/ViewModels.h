#ifndef PIPEFRAME_UI_VIEW_MODELS_H
#define PIPEFRAME_UI_VIEW_MODELS_H

#include <PipeFrame/Foundation/MathTypes.h>

#include <cstdint>
#include <string>

namespace pipeframe {

enum class SemanticColorRole : std::uint8_t { Primary, Secondary, Accent, Success, Warning, Danger };

struct MetricViewModel {
    std::string label;
    std::string value;
    SemanticColorRole role{SemanticColorRole::Primary};
};

struct NetworkNodeViewModel {
    std::uint64_t id{};
    Vector2f normalizedPosition{};
    Color color{};
    std::string label;
    float activation{};
};

struct NetworkEdgeViewModel {
    std::uint64_t source{};
    std::uint64_t target{};
    Color color{};
    float weight{};
    bool enabled{true};
};

}  // namespace pipeframe

#endif
