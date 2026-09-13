#ifndef PIPEFRAME_UI_ZEN_MODE_SHELL_H
#define PIPEFRAME_UI_ZEN_MODE_SHELL_H

#include <functional>

#include <PipeFrame/Backend/SFML/UI/OverlayPanel.h>

class ZenModeShell final : public OverlayPanel {
  public:
    using ChangedCallback = std::function<void(bool)>;

    ZenModeShell();

    OverlayPanel &GetPlayfieldLayer();
    OverlayPanel &GetChromeLayer();
    OverlayPanel &GetEssentialLayer();
    const OverlayPanel &GetPlayfieldLayer() const;
    const OverlayPanel &GetChromeLayer() const;
    const OverlayPanel &GetEssentialLayer() const;

    void SetZenMode(bool enabled, bool notify = false);
    void ToggleZenMode();
    bool IsZenMode() const;
    void SetOnZenModeChanged(ChangedCallback callback);

  private:
    OverlayPanel &playfieldLayer;
    OverlayPanel &chromeLayer;
    OverlayPanel &essentialLayer;
    ChangedCallback onZenModeChanged;
    bool zenMode = false;
};

#endif
