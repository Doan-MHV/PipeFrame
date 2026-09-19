#pragma once
#include <PipeFrame/Input/InputEvent.h>

#include <functional>
#include <utility>

namespace pipeframe::ui {
// Hosts route hit-tested pointer presses here and retain capture during dragging.
// This controller owns interaction state; layout owns size limits and persistence.
class DockResizeController {
public:
    enum class Axis { Horizontal, Vertical };
    using DragCallback = std::function<void(float)>;
    explicit DockResizeController(Axis axis) : axis(axis) {}
    void SetAxis(Axis value) {
        if (axis != value) Cancel();
        axis = value;
    }
    void SetOnDragged(DragCallback callback) { onDragged = std::move(callback); }
    bool IsDragging() const { return dragging; }
    void Cancel() { dragging = false; }
    bool HandleEvent(const InputEvent& event) {
        if (event.type == InputEventType::FocusChanged) {
            if (const auto* focus = event.GetIf<FocusInput>(); focus && !focus->focused) Cancel();
        }
        if (event.type == InputEventType::PointerPresenceChanged) {
            if (const auto* presence = event.GetIf<PointerPresenceInput>(); presence && !presence->inside) Cancel();
        }
        if (event.type == InputEventType::KeyPressed) {
            if (const auto* key = event.GetIf<KeyInput>(); key && key->key == InputKey::Escape && dragging) {
                Cancel();
                return true;
            }
        }
        if (event.type == InputEventType::PointerPressed) {
            if (const auto* pointer = event.GetIf<PointerInput>(); pointer && pointer->button == PointerButton::Left) {
                previous = pointer->position;
                dragging = true;
                return true;
            }
        }
        if (event.type == InputEventType::PointerMoved && dragging) {
            if (const auto* pointer = event.GetIf<PointerMoveInput>()) {
                const float delta = axis == Axis::Horizontal ? float(pointer->position.x) - float(previous.x)
                                                             : float(pointer->position.y) - float(previous.y);
                previous = pointer->position;
                if (delta != 0 && onDragged) onDragged(delta);
                return true;
            }
        }
        if (event.type == InputEventType::PointerReleased && dragging) {
            if (const auto* pointer = event.GetIf<PointerInput>(); pointer && pointer->button == PointerButton::Left) {
                Cancel();
                return true;
            }
        }
        return false;
    }

private:
    Axis axis;
    DragCallback onDragged;
    Vector2i previous{};
    bool dragging{};
};
}  // namespace pipeframe::ui
