#include <PipeFrame/UI/DockResizeController.h>
#include <PipeFrame/UI/FloatingWindowController.h>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace pipeframe;
using pipeframe::ui::DockResizeController;
void Require(bool value) { if (!value) { std::cerr << "Dock resize regression failed\n"; std::exit(1); } }
int main() {
    DockResizeController controller(DockResizeController::Axis::Horizontal);
    std::vector<float> deltas;
    controller.SetOnDragged([&](float delta) { deltas.push_back(delta); });
    auto press = [&](PointerButton button) { return controller.HandleEvent({InputEventType::PointerPressed, PointerInput{button,{10,20}}}); };
    auto move = [&](int x, int y) { return controller.HandleEvent({InputEventType::PointerMoved, PointerMoveInput{{x,y}}}); };
    Require(!move(15,30) && !press(PointerButton::Right));
    Require(press(PointerButton::Left) && controller.IsDragging());
    Require(move(15,30) && move(15,50) && move(8,50));
    Require(deltas == std::vector<float>({5,-7}));
    Require(!controller.HandleEvent({InputEventType::PointerReleased, PointerInput{PointerButton::Right,{}}}));
    Require(controller.IsDragging());
    Require(controller.HandleEvent({InputEventType::PointerReleased, PointerInput{PointerButton::Left,{}}}));
    Require(!move(100,100));
    controller.SetAxis(DockResizeController::Axis::Vertical);
    Require(press(PointerButton::Left) && move(999,25) && deltas.back()==5);
    controller.SetAxis(DockResizeController::Axis::Horizontal);
    Require(!controller.IsDragging() && !move(0,0));
    for (const InputEvent cancellation : {
        InputEvent{InputEventType::FocusChanged,FocusInput{false}},
        InputEvent{InputEventType::PointerPresenceChanged,PointerPresenceInput{false}},
        InputEvent{InputEventType::KeyPressed,KeyInput{InputKey::Escape}}}) {
        Require(press(PointerButton::Left));
        controller.HandleEvent(cancellation);
        Require(!controller.IsDragging() && !move(900,900));
    }
    // Malformed payloads must not trigger a drag or crash a neutral host.
    Require(!controller.HandleEvent({InputEventType::PointerPressed,FocusInput{true}}));
    using Window = ui::FloatingWindowController;
    Window window;
    window.Begin(Window::Operation::Move,{10,20},{{80,90},{400,300}});
    Require(window.Update({40,60},{1100,800}) == Rectanglef{{110,130},{400,300}});
    Require(window.Update({-1000,-1000},{1100,800}).position == Vector2f{8,8});
    window.Begin(Window::Operation::Resize,{0,0},{{80,90},{400,300}});
    Require(window.Update({100,50},{1100,800}).size == Vector2f{500,350});
    Require(window.Update({-1000,-1000},{1100,800}).size == Vector2f{360,240});
    const auto small=window.Update({1000,1000},{200,120});
    Require(small == Rectanglef{{8,8},{184,104}});
    Require(Window::Constrain({{90,90},{400,300}},{1,1}) == Rectanglef{{0,0},{1,1}});
    window.End(); Require(!window.IsActive());
    std::cout << "Dock resize regression passed\n";
}
