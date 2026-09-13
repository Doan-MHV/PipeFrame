#pragma once
#include <optional>
#include <PipeFrame/Input/InputEvent.h>
class RenderContext;

class CameraController2D {
public:
    void HandleEvent(const pipeframe::InputEvent &event, RenderContext &context);
    void CancelInteraction();
    bool IsPanModifierActive() const { return spacePressed; }
private:
    std::optional<pipeframe::PointerButton> dragButton;
    pipeframe::Vector2i previousMousePosition{};
    bool spacePressed{};
    static constexpr float MinimumZoom = 0.1f;
    static constexpr float MaximumZoom = 10.0f;
    static constexpr float ZoomFactor = 0.85f;
};
