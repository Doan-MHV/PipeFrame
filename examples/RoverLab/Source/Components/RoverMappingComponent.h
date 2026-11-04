#pragma once

#include <PipeFrame/Project/Component.h>

#include <cmath>
#include <cstdint>

struct RoverMappingComponent : pipeframe::Component<RoverMappingComponent> {
  bool enabled{true};
  double cellSize{0.10};
  std::int64_t sideCells{160};
  double rangeMargin{0.02};

  bool operator==(const RoverMappingComponent&) const = default;

  bool Valid() const {
    return std::isfinite(cellSize) && cellSize >= 0.02 && cellSize <= 1 && sideCells >= 16 &&
           sideCells <= 256 && std::isfinite(rangeMargin) && rangeMargin >= 0 && rangeMargin <= 1;
  }

  static auto BuildSchema() {
    using namespace pipeframe;

    return Define("project.RoverMappingComponent", "Rover Mapping")
      .Editable("enabled", "Enabled", &RoverMappingComponent::enabled)
      .Editable("cellSize", "Cell Size", &RoverMappingComponent::cellSize, {.unit = "metres", .minimum = 0.02, .maximum = 1.0})
      .Editable("sideCells", "Initial Cells Per Side", &RoverMappingComponent::sideCells, {.minimum = 16, .maximum = 256})
      .Editable("rangeMargin", "Extra Range Uncertainty", &RoverMappingComponent::rangeMargin, {.unit = "metres", .minimum = 0, .maximum = 1})
      .Validate("Invalid mapping dimensions or uncertainty",
        [](const auto& value) { return value.Valid(); });
  }
};
