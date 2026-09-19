#pragma once
#include <PipeFrame/Environment/Tilemap2D.h>
#include <PipeFrame/Project/ComponentSchema.h>

namespace pipeframe {

inline constexpr const char* PlaygroundComponentTypeId = "pipeframe.playground";
inline constexpr const char* PlaygroundEntityTypeId = "pipeframe.playground2d";

struct PlaygroundComponent {
    std::int64_t columns{32}, rows{24};
    float cellSize{10};
    AssetReference material;
    Color color{40, 46, 52, 255};
    bool showGrid{true};
    Vector2f Size() const { return {float(columns) * cellSize, float(rows) * cellSize}; }
    static auto Schema() {
        return ComponentSchema<PlaygroundComponent>(PlaygroundComponentTypeId, "Playground")
            .Editable({.key = "columns",
                       .displayName = "Columns",
                       .kind = PropertyKind::Integer,
                       .defaultValue = std::int64_t{32},
                       .minimum = 1,
                       .maximum = 2048},
                      &PlaygroundComponent::columns)
            .Editable({.key = "rows",
                       .displayName = "Rows",
                       .kind = PropertyKind::Integer,
                       .defaultValue = std::int64_t{24},
                       .minimum = 1,
                       .maximum = 2048},
                      &PlaygroundComponent::rows)
            .Editable({.key = "cellSize",
                       .displayName = "Cell Size",
                       .kind = PropertyKind::Number,
                       .defaultValue = 10.0,
                       .unit = "local units",
                       .minimum = 0.001,
                       .maximum = 1000},
                      &PlaygroundComponent::cellSize)
            .Editable({.key = "material",
                       .displayName = "Ground Material",
                       .kind = PropertyKind::AssetReference,
                       .defaultValue = AssetReference{},
                       .editorHint = "asset:Material"},
                      &PlaygroundComponent::material)
            .Editable({.key = "color",
                       .displayName = "Ground Color",
                       .kind = PropertyKind::Color,
                       .defaultValue = Color{40, 46, 52, 255}},
                      &PlaygroundComponent::color)
            .Editable({.key = "showGrid",
                       .displayName = "Show Cell Grid",
                       .kind = PropertyKind::Boolean,
                       .defaultValue = true},
                      &PlaygroundComponent::showGrid)
            .ReadOnly({.key = "size",
                       .displayName = "Local Size",
                       .kind = PropertyKind::Vector2,
                       .defaultValue = Vector2f{320, 240}},
                      [](const auto& value) -> PropertyValue { return value.Size(); })
            .Validate("Playground exceeds the tilemap cell budget", [](const auto& value) {
                return value.columns > 0 && value.rows > 0 &&
                       std::uint64_t(value.columns) <= Tilemap2D::MaxCells / std::uint64_t(value.rows);
            });
    }
};
}  // namespace pipeframe
