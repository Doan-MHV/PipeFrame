#include <PipeFrame/Render/CameraController2D.h>
#include <PipeFrame/Render/RenderContext.h>
#include <algorithm>
#include <cmath>

void CameraController2D::CancelInteraction() { dragButton.reset(); spacePressed=false; }

void CameraController2D::HandleEvent(const pipeframe::InputEvent &event, RenderContext &context) {
    using namespace pipeframe;
    if(event.type==InputEventType::FocusChanged) {
        if(const auto *focus=event.GetIf<FocusInput>();focus&&!focus->focused) CancelInteraction();
    }
    if(const auto *key=event.GetIf<KeyInput>();key&&key->key==InputKey::Space) {
        if(event.type==InputEventType::KeyPressed) spacePressed=true;
        if(event.type==InputEventType::KeyReleased) spacePressed=false;
    }
    if(event.type==InputEventType::PointerPressed) {
        if(const auto *pressed=event.GetIf<PointerInput>();pressed&&
            (pressed->button==PointerButton::Middle||(pressed->button==PointerButton::Left&&spacePressed))) {
            dragButton=pressed->button;previousMousePosition=pressed->position;
        }
    }
    if(event.type==InputEventType::PointerReleased) {
        if(const auto *released=event.GetIf<PointerInput>();released&&dragButton==released->button) dragButton.reset();
    }
    if(event.type==InputEventType::PointerMoved) {
        if(const auto *moved=event.GetIf<PointerMoveInput>()) {
            if(dragButton) context.GetCamera().Move(context.MapPixelToWorld(previousMousePosition)-context.MapPixelToWorld(moved->position));
            previousMousePosition=moved->position;
        }
    }
    if(event.type==InputEventType::WheelScrolled) {
        if(const auto *wheel=event.GetIf<WheelInput>();wheel&&!wheel->horizontal&&std::isfinite(wheel->delta)) {
            auto &camera=context.GetCamera();
            const auto before=context.MapPixelToWorld(wheel->position);
            camera.SetZoom(std::clamp(camera.GetZoom()*std::pow(ZoomFactor,wheel->delta),MinimumZoom,MaximumZoom));
            camera.Move(before-context.MapPixelToWorld(wheel->position));
        }
    }
}
