#ifndef ANT_COLONY_SETTINGS_H
#define ANT_COLONY_SETTINGS_H
#include <PipeFrame/Project/Component.h>

#include "Integration/AntTypeIds.h"

namespace ant_simulation {
struct ColonySettingsComponent : pipeframe::Component<ColonySettingsComponent> {
  std::int64_t population{1000};
  double radius{4.0};
  double speed{2.0};
  std::int64_t seed{1};
  std::int64_t red{239}, green{71}, blue{111};

  static auto BuildSchema() {
    return Define(ColonyTypeId, "Colony Settings")
      .Required()
      .Editable(InitialPopulationKey, "Initial Population", &ColonySettingsComponent::population, {.minimum = 0, .maximum = 1'000'000, .step = 1})
      .Editable(SpawnRadiusKey, "Colony Radius", &ColonySettingsComponent::radius, {.unit = "m", .minimum = 0.5, .maximum = 64, .step = 0.25})
      .Editable(MovementSpeedKey, "Ant Speed", &ColonySettingsComponent::speed, {.unit = "m/s", .minimum = 0.01, .maximum = 100, .step = 0.1})
      .Editable(RandomSeedKey, "Random Seed", &ColonySettingsComponent::seed, {.minimum = 0, .maximum = 4'294'967'295.0, .step = 1})
      .Editable(ColonyColorRedKey, "Color Red", &ColonySettingsComponent::red, {.minimum = 0, .maximum = 255, .step = 1})
      .Editable(ColonyColorGreenKey, "Color Green", &ColonySettingsComponent::green, {.minimum = 0, .maximum = 255, .step = 1})
      .Editable(ColonyColorBlueKey, "Color Blue", &ColonySettingsComponent::blue, {.minimum = 0, .maximum = 255, .step = 1});
  }
};
}  // namespace ant_simulation
#endif
