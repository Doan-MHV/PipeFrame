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
      .Editable(
        {
          .key = "enabled",
          .displayName = "Enabled",
          .kind = PropertyKind::Boolean,
          .defaultValue = true,
        },
        &RoverMappingComponent::enabled)
      .Editable(
        {
          .key = "cellSize",
          .displayName = "Cell Size",
          .kind = PropertyKind::Number,
          .defaultValue = 0.10,
          .unit = "metres",
          .minimum = 0.02,
          .maximum = 1.0,
        },
        &RoverMappingComponent::cellSize)
      .Editable(
        {
          .key = "sideCells",
          .displayName = "Initial Cells Per Side",
          .kind = PropertyKind::Integer,
          .defaultValue = std::int64_t{160},
          .minimum = 16,
          .maximum = 256,
        },
        &RoverMappingComponent::sideCells)
      .Editable(
        {
          .key = "rangeMargin",
          .displayName = "Extra Range Uncertainty",
          .kind = PropertyKind::Number,
          .defaultValue = 0.02,
          .unit = "metres",
          .minimum = 0,
          .maximum = 1,
        },
        &RoverMappingComponent::rangeMargin)
      .Validate("Invalid mapping dimensions or uncertainty",
        [](const auto& value) { return value.Valid(); });
  }
};
