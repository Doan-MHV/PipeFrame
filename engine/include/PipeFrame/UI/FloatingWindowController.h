#pragma once
#include <PipeFrame/Foundation/MathTypes.h>
#include <algorithm>

namespace pipeframe::ui {
// Hosts own titlebar/grip hit testing, input capture and persistence.
class FloatingWindowController {
public:
    enum class Operation { Move, Resize };
    static Rectanglef Constrain(Rectanglef bounds, Vector2f viewport,
                                Vector2f minimum = {360,240}, float margin = 8) {
        const auto constrainAxis = [margin](float &position, float &size, float extent, float minSize) {
            extent = std::max(1.0f, extent);
            const float inset = std::min(std::max(0.0f, margin), (extent-1)*0.5f);
            const float available = extent-2*inset;
            size = std::clamp(size, std::min(std::max(1.0f,minSize),available), available);
            position = std::clamp(position, inset, extent-size-inset);
        };
        constrainAxis(bounds.position.x,bounds.size.x,viewport.x,minimum.x);
        constrainAxis(bounds.position.y,bounds.size.y,viewport.y,minimum.y);
        return bounds;
    }
    void Begin(Operation value, Vector2f pointer, Rectanglef bounds) {
        operation=value; startPointer=pointer; startBounds=bounds; active=true;
    }
    bool IsActive() const { return active; }
    void End() { active=false; }
    Rectanglef Update(Vector2f pointer, Vector2f viewport) const {
        auto bounds=startBounds;
        if (active) {
            const auto delta=pointer-startPointer;
            if(operation==Operation::Move) bounds.position+=delta;
            else bounds.size+=delta;
        }
        return Constrain(bounds,viewport);
    }
private:
    Operation operation{Operation::Move};
    Vector2f startPointer{};
    Rectanglef startBounds{};
    bool active{};
};
}
