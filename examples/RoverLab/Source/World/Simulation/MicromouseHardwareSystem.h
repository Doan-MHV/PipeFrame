#pragma once

#include <PipeFrame/Components/EnvironmentCollider2DComponent.h>
#include <PipeFrame/Components/SpriteRendererComponent.h>
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Simulation/System.h>

#include "Components/MicromouseHardwareComponent.h"
#include "Components/RoverSafetyComponent.h"

namespace RoverLab {

// Hardware is the geometry authority only on rovers that opt into this component.
class MicromouseHardwareSystem final : public pipeframe::FixedUpdateSystem<void> {
public:
  explicit MicromouseHardwareSystem(pipeframe::BehaviourScene& scene) : scene(scene) {}

  std::string_view GetSystemId() const override { return "roverlab.micromouse-hardware"; }
  void Update(float) override { Apply(); }

  void Apply() {
    auto& components = scene.Components();
    components.Each<MicromouseHardwareComponent>([&](pipeframe::ecs::Entity entity) {
      const auto& hardware = *components.Get<MicromouseHardwareComponent>(entity);
      if (!hardware.Valid()) return;

      if (auto* collider = components.Get<pipeframe::EnvironmentCollider2DComponent>(entity)) {
        collider->shape = "Box";
        collider->size = {float(hardware.length), float(hardware.width)};
      }
      if (auto* safety = components.Get<RoverSafetyComponent>(entity)) {
        safety->halfLength = hardware.length / 2;
        safety->halfWidth = hardware.width / 2;
      }
      if (auto* sprite = components.Get<pipeframe::SpriteRendererComponent>(entity))
        sprite->size = {float(hardware.length), float(hardware.width)};
    });
  }

private:
  pipeframe::BehaviourScene& scene;
};

}  // namespace RoverLab
