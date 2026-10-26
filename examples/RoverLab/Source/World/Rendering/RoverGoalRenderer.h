#pragma once

#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Render/GeometryDraw.h>
#include <PipeFrame/Render/RenderSystem.h>

#include "Components/RoverGoalComponent.h"

namespace RoverLab {

class RoverGoalRenderer final : public pipeframe::RenderSystem {
protected:
  void Draw(pipeframe::Canvas canvas, pipeframe::BehaviourScene& scene) override {
    pipeframe::GeometryDraw draw;

    scene.Components().Each<RoverGoalComponent, pipeframe::Transform2DComponent>(
      [&](pipeframe::ecs::Entity entity) {
        if (!scene.IsActive(entity)) return;

        const auto& goal = *scene.Components().Get<RoverGoalComponent>(entity);
        const auto& pose = *scene.Components().Get<pipeframe::Transform2DComponent>(entity);
        if (!goal.enabled || !goal.Valid()) return;

        const auto p = pose.position;
        const float r = float(goal.radius);
        const pipeframe::Color color{70, 240, 120, 255};

        draw.Circle(p, r, color);
        draw.Line({p.x - r * .5f, p.y}, {p.x + r * .5f, p.y}, color);
        draw.Line({p.x, p.y - r * .5f}, {p.x, p.y + r * .5f}, color);
      });

    draw.Draw(canvas);
  }
};

}  // namespace RoverLab
