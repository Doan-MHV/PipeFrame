#pragma once
#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/UI/DockResizeController.h>

namespace pipeframe::backend::sfml {
// Native host only. Reusable resize behaviour lives in DockResizeController.
class DockSplitter final : public Panel {
public:
    using Axis = ui::DockResizeController::Axis;
    using DragCallback = ui::DockResizeController::DragCallback;
    explicit DockSplitter(Axis axis);
    void SetAxis(Axis axis);
    void SetOnDragged(DragCallback callback);
    bool IsDragging() const;
protected:
    bool OnEvent(const sf::Event &event) override;
    void OnPointerEntered() override;
    void OnPointerExited() override;
private:
    void RefreshColor();
    ui::DockResizeController controller;
    bool hovered{};
};
}
