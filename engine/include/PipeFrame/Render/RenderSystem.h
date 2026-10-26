#pragma once
#include <PipeFrame/Render/Canvas.h>

namespace pipeframe {
class BehaviourScene;

struct RenderOptions {
    bool physics{};
    bool mesh{};
};

enum class RenderVisibility { Always, Physics, Mesh };

// Scene presentation only. Advance animation/simulation state in update systems.
class RenderSystem {
public:
    virtual ~RenderSystem() = default;
    void SetEnabled(bool value) { enabled = value; }
    void SetVisibility(RenderVisibility value) { visibility = value; }
    RenderVisibility GetVisibility() const { return visibility; }
    void Render(Canvas canvas, BehaviourScene& scene, RenderOptions options = {}) {
        if (!enabled) return;
        if (visibility == RenderVisibility::Physics && !options.physics) return;
        if (visibility == RenderVisibility::Mesh && !options.mesh) return;
        Draw(canvas, scene);
    }

protected:
    virtual void Draw(Canvas canvas, BehaviourScene& scene) = 0;

private:
    bool enabled{true};
    RenderVisibility visibility{RenderVisibility::Always};
};
} // namespace pipeframe
