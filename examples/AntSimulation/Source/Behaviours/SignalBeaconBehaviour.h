#pragma once
#include "Components/SignalBeaconComponent.h"
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/ECS/Scene.h>
#include <cmath>
#include <numbers>
namespace ant_simulation {
// Uses the shared scene's fixed lifecycle. Does not participate in ant foraging.
class SignalBeaconBehaviour final : public pipeframe::Behaviour {
  public:
    void FixedUpdate(float delta) override {
        const auto *settings = GetComponent<SignalBeaconComponent>();
        auto *transform = GetComponent<pipeframe::Transform2DComponent>();
        if (!settings || !transform)
            return;
        transform->rotation = std::remainder(
            transform->rotation + static_cast<float>(settings->rotationSpeed * std::numbers::pi / 180.0) * delta,
            2 * std::numbers::pi_v<float>);
    }
};
} // namespace ant_simulation
