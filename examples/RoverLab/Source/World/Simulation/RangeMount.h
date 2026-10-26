#pragma once
#include <PipeFrame/ECS/Scene.h>

#include <optional>

#include "Components/InfraredSensorComponent.h"
#include "Components/UltrasonicSensorComponent.h"
namespace RoverLab {
struct RangeMount {
  pipeframe::SceneObjectReference parent;
  pipeframe::Vector2f localPosition;
  double localAngleDegrees{}, minimumRange{}, maximumRange{}, coneAngleDegrees{};
  bool enabled{true};
  bool operator==(const RangeMount&) const = default;
  template <class T>
  static RangeMount From(const T& s) {
    return {s.parent,
      s.localPosition,
      s.localAngleDegrees,
      s.minimumRange,
      s.maximumRange,
      s.coneAngleDegrees,
      s.enabled};
  }
};

inline std::optional<RangeMount> ReadRangeMount(pipeframe::SceneObject object) {
  if (!object.IsValid()) return std::nullopt;
  const auto* sonar = object.GetComponent<UltrasonicSensorComponent>();
  const auto* optical = object.GetComponent<InfraredSensorComponent>();
  if (sonar && optical) return std::nullopt;
  if (sonar) return RangeMount::From(*sonar);
  if (optical && optical->Valid()) return RangeMount::From(*optical);
  return std::nullopt;
}
}  // namespace RoverLab
