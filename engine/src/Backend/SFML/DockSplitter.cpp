#include <PipeFrame/Backend/SFML/DockSplitter.h>
#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

namespace pipeframe::backend::sfml {
DockSplitter::DockSplitter(Axis axis) : controller(axis) {
    SetFocusable(true);
    SetOutlineThickness(0);
    RefreshColor();
}
void DockSplitter::SetAxis(Axis axis) {
    controller.SetAxis(axis);
    RefreshColor();
}
void DockSplitter::SetOnDragged(DragCallback callback) { controller.SetOnDragged(std::move(callback)); }
bool DockSplitter::IsDragging() const { return controller.IsDragging(); }
bool DockSplitter::OnEvent(const sf::Event &event) {
    const auto input = FromBackend(event);
    const bool handled = input && controller.HandleEvent(*input);
    if (event.is<sf::Event::FocusLost>() || event.is<sf::Event::MouseLeft>())
        hovered = false;
    RefreshColor();
    return handled;
}
void DockSplitter::OnPointerEntered() {
    hovered = true;
    RefreshColor();
}
void DockSplitter::OnPointerExited() {
    hovered = false;
    RefreshColor();
}
void DockSplitter::RefreshColor() {
    SetFillColor(hovered || controller.IsDragging() ? UITheme::Dark().accentHovered : UITheme::Dark().subtleBorder);
}
} // namespace pipeframe::backend::sfml
