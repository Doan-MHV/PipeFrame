#include <PipeFrame/Backend/SFML/SimulationDashboardHost.h>
#include "ViewRenderer.h"

Widget &NativeSimulationDashboard::RenderView(Widget &parent, const pipeframe::ui::View &view) {
    pipeframe::ui::ValidateView(view);
    return pipeframe::ui::ViewRenderer(font, theme).Render(parent, view);
}

#include "MountedViewHost.h"
pipeframe::ui::MountedView UIManager::MountView(pipeframe::ui::View view, const UITheme &theme) {
    pipeframe::ui::ValidateView(view);
    if (!HasDefaultFont()) throw std::logic_error("Load a UI font before mounting views");
    auto state=std::make_shared<pipeframe::ui::detail::ViewMountState>();
    state->view=std::move(view);
    CreateRoot<pipeframe::ui::MountedViewHost>(state,GetDefaultFont(),theme);
    return pipeframe::ui::MountedView(state);
}
