#include <PipeFrame/Backend/SFML/ViewPanelHost.h>
#include "MountedViewHost.h"

namespace pipeframe::ui {
NativeViewPanel::NativeViewPanel(const sf::Font &font, const UITheme &theme)
    : revision(0,[this](const std::uint64_t &){return BuildNativeView();}),
      state(std::make_shared<detail::ViewMountState>(detail::ViewMountState{revision.Describe("view").FillHeight()})), mount(state) {
    SetFillColor(theme.surface); SetOutlineColor(theme.border);
    CreateChild<MountedViewHost>(state,font,theme);
}
NativeViewPanel::~NativeViewPanel() { mount.Unmount(); }
void NativeViewPanel::InvalidateView() { dirty=true; }
void NativeViewPanel::OnUpdate(float) {
    if (dirty) { revision.SetState([](auto &value){++value;}); dirty=false; }
    mount.SetBounds(0,0,GetSize().x,GetSize().y);
}
void NativeViewPanel::OnGeometryChanged() { Panel::OnGeometryChanged(); InvalidateView(); }
}
