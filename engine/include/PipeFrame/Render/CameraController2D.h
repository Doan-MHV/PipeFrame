#pragma once
#include <PipeFrame/Input/InputEvent.h>

#include <optional>
class RenderContext;

class CameraController2D {
public:
    void HandleEvent(const pipeframe::InputEvent& event, RenderContext& context);
    void CancelInteraction();
    bool IsPanModifierActive() const { return spacePressed; }
    bool IsPanning() const { return dragButton.has_value(); }

    static constexpr float MinimumZoom = 0.00001f;
    static constexpr float MaximumZoom = 10000.0f;

private:
    std::optional<pipeframe::PointerButton> dragButton;
    pipeframe::Vector2i previousMousePosition{};
    bool spacePressed{};
    static constexpr float ZoomFactor = 0.85f;
};
