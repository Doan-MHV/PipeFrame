#include <PipeFrame/Backend/SFML/UI/ZenModeShell.h>

#include <utility>

ZenModeShell::ZenModeShell()
    : playfieldLayer(CreateChild<OverlayPanel>()), chromeLayer(CreateChild<OverlayPanel>()),
      essentialLayer(CreateChild<OverlayPanel>()) {
    SetFillColor(sf::Color::Transparent);
    SetOutlineColor(sf::Color::Transparent);
    SetOutlineThickness(0.0f);
    SetHitTestVisible(false);

    for (OverlayPanel *layer : {&playfieldLayer, &chromeLayer, &essentialLayer}) {
        layer->SetFillColor(sf::Color::Transparent);
        layer->SetOutlineThickness(0.0f);
        layer->SetHitTestVisible(false);
        SetChildAlignment(*layer, {HorizontalAlignment::Stretch, VerticalAlignment::Stretch});
    }
}

OverlayPanel &ZenModeShell::GetPlayfieldLayer() { return playfieldLayer; }
OverlayPanel &ZenModeShell::GetChromeLayer() { return chromeLayer; }
OverlayPanel &ZenModeShell::GetEssentialLayer() { return essentialLayer; }
const OverlayPanel &ZenModeShell::GetPlayfieldLayer() const { return playfieldLayer; }
const OverlayPanel &ZenModeShell::GetChromeLayer() const { return chromeLayer; }
const OverlayPanel &ZenModeShell::GetEssentialLayer() const { return essentialLayer; }

void ZenModeShell::SetZenMode(const bool enabled, const bool notify) {
    if (zenMode == enabled) {
        return;
    }
    zenMode = enabled;
    chromeLayer.SetVisible(!zenMode);
    if (notify && onZenModeChanged) {
        onZenModeChanged(zenMode);
    }
}

void ZenModeShell::ToggleZenMode() { SetZenMode(!zenMode, true); }
bool ZenModeShell::IsZenMode() const { return zenMode; }

void ZenModeShell::SetOnZenModeChanged(ChangedCallback callback) { onZenModeChanged = std::move(callback); }
