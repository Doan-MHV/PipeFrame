#pragma once
#include <PipeFrame/Project/Component.h>

#include "Integration/AntTypeIds.h"

namespace ant_simulation {
struct FoodSourceComponent : pipeframe::Component<FoodSourceComponent> {
  std::int64_t amount{7};
  double radius{8.0};

  static auto BuildSchema() {
    using namespace pipeframe;
    return Define(FoodSourceTypeId, "Food Source Settings")
      .Required()
      .Editable(FoodAmountKey, "Food Per Cell", &FoodSourceComponent::amount, {.minimum = 0, .maximum = 1'000'000, .step = 1})
      .Editable(FoodRadiusKey, "Radius", &FoodSourceComponent::radius, {.unit = "m", .minimum = 0.25, .maximum = 256.0, .step = 0.25});
  }
};
}  // namespace ant_simulation
