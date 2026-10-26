#pragma once
#include <PipeFrame/Project/Component.h>

#include "Integration/AntTypeIds.h"

namespace ant_simulation {
struct FoodSourceComponent : pipeframe::Component<FoodSourceComponent> {
  std::int64_t amount{7};
  double radius{8.0};

  static auto BuildSchema() {
    using namespace pipeframe;
    using K = PropertyKind;
    return Define(FoodSourceTypeId, "Food Source Settings")
      .Required()
      .Editable({.key = FoodAmountKey,
                  .displayName = "Food Per Cell",
                  .kind = K::Integer,
                  .defaultValue = std::int64_t{7},
                  .unit = "",
                  .minimum = 0,
                  .maximum = 1'000'000,
                  .step = 1},
        &FoodSourceComponent::amount)
      .Editable({.key = FoodRadiusKey,
                  .displayName = "Radius",
                  .kind = K::Number,
                  .defaultValue = 8.0,
                  .unit = "m",
                  .minimum = 0.25,
                  .maximum = 256.0,
                  .step = 0.25},
        &FoodSourceComponent::radius);
  }
};
}  // namespace ant_simulation
