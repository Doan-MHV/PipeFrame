#pragma once

#include <PipeFrame/Input/InputEvent.h>
#include <PipeFrame/Render/GeometryDraw.h>
#include <PipeFrame/Render/RenderContext.h>

#include <optional>
#include <unordered_map>

#include "World/Simulation/RoverSolverSystem.h"

namespace RoverLab {

// Scene coordinates are only an input/display frame. Planning still uses the learned map.
class RoverDestinationTool {
public:
  struct Frame {
    pipeframe::SceneObject rover;
    pipeframe::Vector2f origin;
    float rotation{};

    MazePoint ToMap(pipeframe::Vector2f point) const {
      const auto delta = point - origin;
      const double c = std::cos(rotation), s = std::sin(rotation);
      return {c * delta.x + s * delta.y, -s * delta.x + c * delta.y};
    }

    pipeframe::Vector2f ToScene(MazePoint point) const {
      const double c = std::cos(rotation), s = std::sin(rotation);
      return origin + pipeframe::Vector2f{
                        float(c * point.x - s * point.y), float(s * point.x + c * point.y)};
    }
  };

  void Reset() {
    Cancel();
    frames.clear();
  }

  // Capture once, before the first physics tick, matching odometry's starting frame.
  void CaptureFrames(pipeframe::BehaviourScene& scene) {
    std::erase_if(frames, [](const auto& entry) { return !entry.second.rover.IsValid(); });
    scene.Components().Each<RoverSolverComponent, pipeframe::Transform2DComponent>(
      [&](pipeframe::ecs::Entity entity) {
        if (!scene.IsActive(entity)) return;
        const auto object = scene.GetObject(entity);
        const auto found = frames.find(entity);
        if (found != frames.end() && found->second.rover == object) return;
        const auto& pose = *object.GetComponent<pipeframe::Transform2DComponent>();
        frames[entity] = {object, pose.position, pose.rotation};
      });
  }

  bool Arm(pipeframe::ecs::Entity entity, const RoverSolverSystem& solver) {
    const auto* mission = solver.Mission(entity);
    if (!mission || !mission->CanSelectDestination() || !frames.contains(entity)) return false;
    Cancel();
    target = entity;
    return true;
  }

  void Cancel() {
    target.reset();
    press.reset();
    hover.reset();
  }
  void ClearPointer() {
    press.reset();
    hover.reset();
  }
  bool IsArmed(pipeframe::ecs::Entity entity) const { return target == entity; }
  bool HasCapture() const { return press.has_value(); }

  bool Handle(
    const pipeframe::InputEvent& event, RenderContext& context, RoverSolverSystem& solver) {
    using namespace pipeframe;
    if (!target) return false;
    const auto* mission = solver.Mission(*target);
    if (!mission || !mission->CanSelectDestination()) {
      Cancel();
      return false;
    }

    if (event.type == InputEventType::FocusChanged ||
        event.type == InputEventType::PointerPresenceChanged) {
      ClearPointer();
      return false;
    }
    if (const auto* key = event.GetIf<KeyInput>();
      key && event.type == InputEventType::KeyPressed && key->key == InputKey::Escape) {
      Cancel();
      return true;
    }
    if (const auto* move = event.GetIf<PointerMoveInput>()) {
      hover = context.IsInsideWorldViewport(move->position)
                ? std::optional(context.MapPixelToWorld(move->position))
                : std::nullopt;
      if (press &&
          (std::abs(move->position.x - press->x) > 4 || std::abs(move->position.y - press->y) > 4))
        dragged = true;
      return hover.has_value() || press.has_value();
    }
    const auto* pointer = event.GetIf<PointerInput>();
    if (!pointer || pointer->button != PointerButton::Left) return false;
    if (event.type == InputEventType::PointerPressed) {
      if (!context.IsInsideWorldViewport(pointer->position)) return false;
      press = pointer->position;
      dragged = false;
      return true;
    }
    if (event.type == InputEventType::PointerReleased && press) {
      const auto start = *press;
      press.reset();
      if (!dragged && std::abs(pointer->position.x - start.x) <= 4 &&
          std::abs(pointer->position.y - start.y) <= 4 &&
          context.IsInsideWorldViewport(pointer->position)) {
        const auto frame = frames.find(*target);
        if (frame != frames.end() &&
            solver.SelectDestination(
              *target, frame->second.ToMap(context.MapPixelToWorld(pointer->position))))
          Cancel();
      }
      return true;
    }
    return false;
  }

  pipeframe::GeometryDraw Geometry(const RoverSolverSystem& solver, float cursorRadius) const {
    pipeframe::GeometryDraw draw;
    const auto cross = [&](pipeframe::Vector2f point, float radius) {
      const pipeframe::Color yellow{255, 215, 45};
      draw.Circle(point, radius, yellow);
      draw.Line(
        point - pipeframe::Vector2f{radius, 0}, point + pipeframe::Vector2f{radius, 0}, yellow);
      draw.Line(
        point - pipeframe::Vector2f{0, radius}, point + pipeframe::Vector2f{0, radius}, yellow);
    };
    for (const auto& [entity, frame] : frames) {
      if (!frame.rover.IsValid()) continue;
      const auto path = solver.PlannedPath(entity);
      for (std::size_t i = 1; i < path.size(); ++i)
        draw.Line(frame.ToScene(path[i - 1]), frame.ToScene(path[i]), {40, 240, 225});
      const auto* mission = solver.Mission(entity);
      if (mission && mission->destination)
        cross(frame.ToScene(*mission->destination), cursorRadius);
    }
    if (target && hover) cross(*hover, cursorRadius);
    return draw;
  }

private:
  std::unordered_map<pipeframe::ecs::Entity, Frame> frames;
  std::optional<pipeframe::ecs::Entity> target;
  std::optional<pipeframe::Vector2i> press;
  std::optional<pipeframe::Vector2f> hover;
  bool dragged{};
};

}  // namespace RoverLab
