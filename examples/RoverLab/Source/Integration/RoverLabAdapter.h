#pragma once
#include <PipeFrame/Project/SceneProjectRuntime.h>

#include "RoverLabProject.h"
namespace RoverLab {
class RoverLabAdapter final : public pipeframe::SceneProjectRuntime {
public:
  RoverLabAdapter() : SceneProjectRuntime("RoverLab"), world(GetRuntimeScene()) {
    // Connect registered systems to the runtime's before/after-physics dispatch.
    SetSimulationWorld(world.simulation);
    const auto project = CreateProjectDefinition();
    RegisterProject(project, world);

    const auto resolver = [this](pipeframe::SceneObjectId id) {
      return ResolveSceneObject(id);
    };

    world.simulation.ultrasonic.SetObjectResolver(resolver);
    world.simulation.lidar.SetObjectResolver(resolver);
    world.simulation.infrared.SetObjectResolver(resolver);
    world.simulation.drive.SetObjectResolver(resolver);
    world.simulation.navigation.SetObjectResolver(resolver);
    world.simulation.mapping.SetObjectResolver(resolver);
    world.simulation.solver.SetObjectResolver(resolver);
  }

  void FixedUpdate(float delta) override {
    world.destinationTool.CaptureFrames(GetRuntimeScene());
    SceneProjectRuntime::FixedUpdate(delta);
  }

  // Native tools use the same runtime and systems as the editor.
  ProjectWorld& GetProjectWorld() { return world; }

  bool HandleUIEvent(const pipeframe::InputEvent& event, RenderContext& context) override {
    if (SceneProjectRuntime::HandleUIEvent(event, context)) {
      world.destinationTool.ClearPointer();
      return true;
    }
    return world.destinationTool.Handle(event, context, world.simulation.solver);
  }

  void HandleEvent(const pipeframe::InputEvent& event, RenderContext& context) override {
    // Captured releases bypass ordinary UI dispatch in the Workbench.
    if (const auto* pointer = event.GetIf<pipeframe::PointerInput>();
      pointer && event.type == pipeframe::InputEventType::PointerReleased &&
      ConsumesPointerAt(pointer->position, context)) {
      world.destinationTool.ClearPointer();
      return;
    }
    world.destinationTool.Handle(event, context, world.simulation.solver);
  }

  bool HasWorldPointerCapture() const override { return world.destinationTool.HasCapture(); }

  void UpdateAuthoringScene() override {
    world.ResetSensorState();
    world.simulation.hardware.Apply();
    world.simulation.ultrasonic.AlignMounts();
    world.simulation.lidar.AlignMounts();
    world.simulation.infrared.AlignMounts();
  }

  void Reset() override {
    world.ResetSensorState();
    SceneProjectRuntime::Reset();
  }

  void Unload() override {
    world.ResetSensorState();
    SceneProjectRuntime::Unload();
  }

  void CollectWorldDebug(
    pipeframe::WorldDebugDraw& draw, pipeframe::WorldDebugOptions options) override {
    SceneProjectRuntime::CollectWorldDebug(draw, options);

    if (options.physics) {
      world.physics.CollectDebug(draw);
    }
  }

  void Render(RenderContext& context) override {
    world.rendering->DrawScene(
      context.GetCanvas(), GetRuntimeScene(), context.GetRenderOptions(), [&] {
        SceneProjectRuntime::Render(context);
      });
    context.BeginWorld();
    const float radius =
      context.GetCameraSize().x / std::max(1, context.GetViewportRectangle().size.x) * 9;
    world.destinationTool.Geometry(world.simulation.solver, radius).Draw(context.GetCanvas());
  }

protected:
  void UpdatePhysics(float delta, const PreviousTransforms& previous) override {
    world.physics.Simulate(
      delta, [&](float step) { SceneProjectRuntime::UpdatePhysics(step, previous); });
  }

private:
  ProjectWorld world;
};
}  // namespace RoverLab
