#ifndef ANT_COLONY_SETTINGS_H
#define ANT_COLONY_SETTINGS_H
#include "Runtime/AntTypeIds.h"
#include <PipeFrame/Project/ComponentSchema.h>

namespace ant_simulation {
struct ColonySettingsComponent {
    std::int64_t population{1000};
    double radius{4.0};
    double speed{2.0};
    std::int64_t seed{1};
    std::int64_t red{239}, green{71}, blue{111};

    static const pipeframe::ComponentSchema<ColonySettingsComponent> &Schema() {
        using K = pipeframe::PropertyKind;
        static const auto schema = pipeframe::ComponentSchema<ColonySettingsComponent>(ColonyTypeId, "Colony Settings")
                                       .Required()
                                       .Editable({.key = InitialPopulationKey,
                                                  .displayName = "Initial Population",
                                                  .kind = K::Integer,
                                                  .defaultValue = std::int64_t{1000},
                                                  .unit = "",
                                                  .minimum = 0,
                                                  .maximum = 1'000'000,
                                                  .step = 1},
                                                 &ColonySettingsComponent::population)
                                       .Editable({.key = SpawnRadiusKey,
                                                  .displayName = "Colony Radius",
                                                  .kind = K::Number,
                                                  .defaultValue = 4.0,
                                                  .unit = "m",
                                                  .minimum = 0.5,
                                                  .maximum = 64,
                                                  .step = 0.25},
                                                 &ColonySettingsComponent::radius)
                                       .Editable({.key = MovementSpeedKey,
                                                  .displayName = "Ant Speed",
                                                  .kind = K::Number,
                                                  .defaultValue = 2.0,
                                                  .unit = "m/s",
                                                  .minimum = 0.01,
                                                  .maximum = 100,
                                                  .step = 0.1},
                                                 &ColonySettingsComponent::speed)
                                       .Editable({.key = RandomSeedKey,
                                                  .displayName = "Random Seed",
                                                  .kind = K::Integer,
                                                  .defaultValue = std::int64_t{1},
                                                  .unit = "",
                                                  .minimum = 0,
                                                  .maximum = 4'294'967'295.0,
                                                  .step = 1},
                                                 &ColonySettingsComponent::seed)
                                       .Editable({.key = ColonyColorRedKey,
                                                  .displayName = "Color Red",
                                                  .kind = K::Integer,
                                                  .defaultValue = std::int64_t{239},
                                                  .unit = "",
                                                  .minimum = 0,
                                                  .maximum = 255,
                                                  .step = 1},
                                                 &ColonySettingsComponent::red)
                                       .Editable({.key = ColonyColorGreenKey,
                                                  .displayName = "Color Green",
                                                  .kind = K::Integer,
                                                  .defaultValue = std::int64_t{71},
                                                  .unit = "",
                                                  .minimum = 0,
                                                  .maximum = 255,
                                                  .step = 1},
                                                 &ColonySettingsComponent::green)
                                       .Editable({.key = ColonyColorBlueKey,
                                                  .displayName = "Color Blue",
                                                  .kind = K::Integer,
                                                  .defaultValue = std::int64_t{111},
                                                  .unit = "",
                                                  .minimum = 0,
                                                  .maximum = 255,
                                                  .step = 1},
                                                 &ColonySettingsComponent::blue);
        return schema;
    }
};
} // namespace ant_simulation
#endif
