#pragma once
#include <PipeFrame/Project/Component.h>
namespace ant_simulation {
inline constexpr const char* SignalBeaconTypeId = "ant.signal-beacon";
struct SignalBeaconComponent : pipeframe::Component<SignalBeaconComponent> {
  double radius{5.0};
  double rotationSpeed{90.0};
  pipeframe::Color color{40, 210, 240, 255};
  static auto BuildSchema() {
    using namespace pipeframe;
    return Define(SignalBeaconTypeId, "Signal Beacon")
      .Required()
      .Editable("radius", "Radius", &SignalBeaconComponent::radius, {.unit = "m", .minimum = 0.5, .maximum = 32, .step = 0.5})
      .Editable("rotationSpeed", "Rotation Speed", &SignalBeaconComponent::rotationSpeed, {.unit = "degrees/s", .minimum = -360, .maximum = 360, .step = 5})
      .Editable("color", "Color", &SignalBeaconComponent::color);
  }
};
}  // namespace ant_simulation
