#pragma once
#include <PipeFrame/World/World.h>
#include <PipeFrame/Render/WorldDebugView.h>
namespace RoverLab {
class ProjectPhysicsWorld final : public pipeframe::PhysicsWorld {
public:
    void CollectDebug(pipeframe::WorldDebugDraw &draw) const {
        // Submit actual body shapes: draw.Circle(position, radius), draw.Polygon(corners).
        (void)draw;
    }
    // Own physics systems here; register ordered updates with AddStep in the constructor.
    // Replace defaultPhysics(delta) below when this project needs a custom solver.
    template<class Step> void Simulate(float delta, Step defaultPhysics) {
        defaultPhysics(delta);
        Update(delta);
    }
};
}
