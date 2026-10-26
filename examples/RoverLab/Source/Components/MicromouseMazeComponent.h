#pragma once
#include <PipeFrame/Project/Component.h>

struct MicromouseMazeComponent : pipeframe::Component<MicromouseMazeComponent> {
  std::int64_t columns{8}, rows{8};
  double cellPitch{.35}, wallThickness{.025};
  bool enabled{true};
  bool operator==(const MicromouseMazeComponent&) const = default;
  bool Valid() const {
    return columns >= 2 && rows >= 2 && columns <= 32 && rows <= 32 && std::isfinite(cellPitch) &&
           cellPitch >= .1 && cellPitch <= 1 && std::isfinite(wallThickness) && wallThickness > 0 &&
           wallThickness < cellPitch / 4;
  }
  static auto BuildSchema() {
    using namespace pipeframe;
    return Define("project.MicromouseMazeComponent", "Micromouse Cell Maze")
      .OnlyOn("project.RoverEntity")
      .Editable({.key = "columns",
                  .displayName = "Columns",
                  .kind = PropertyKind::Integer,
                  .defaultValue = std::int64_t{8},
                  .minimum = 2,
                  .maximum = 32},
        &MicromouseMazeComponent::columns)
      .Editable({.key = "rows",
                  .displayName = "Rows",
                  .kind = PropertyKind::Integer,
                  .defaultValue = std::int64_t{8},
                  .minimum = 2,
                  .maximum = 32},
        &MicromouseMazeComponent::rows)
      .Editable({.key = "cellPitch",
                  .displayName = "Cell Pitch",
                  .kind = PropertyKind::Number,
                  .defaultValue = .35,
                  .unit = "metres",
                  .minimum = .1,
                  .maximum = 1},
        &MicromouseMazeComponent::cellPitch)
      .Editable({.key = "wallThickness",
                  .displayName = "Wall Thickness",
                  .kind = PropertyKind::Number,
                  .defaultValue = .025,
                  .unit = "metres",
                  .minimum = .001,
                  .maximum = .2},
        &MicromouseMazeComponent::wallThickness)
      .Editable({.key = "enabled",
                  .displayName = "Cell Flood Fill Enabled",
                  .kind = PropertyKind::Boolean,
                  .defaultValue = true},
        &MicromouseMazeComponent::enabled)
      .Validate("Valid regular cell geometry required", [](const auto& c) { return c.Valid(); });
  }
};
