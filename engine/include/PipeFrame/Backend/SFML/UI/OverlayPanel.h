#ifndef PIPEFRAME_OVERLAY_PANEL_H
#define PIPEFRAME_OVERLAY_PANEL_H

#include <PipeFrame/Backend/SFML/UI/Panel.h>

#include <unordered_map>

class OverlayPanel : public Panel {
public:
    void SetPadding(Thickness padding);
    Thickness GetPadding() const;

    void SetChildAlignment(Widget& child, Alignment alignment);
    Alignment GetChildAlignment(const Widget& child) const;

    void RefreshLayout();

protected:
    void OnGeometryChanged() override;
    void OnChildGeometryChanged(Widget& child) override;
    void OnChildRemoved(Widget& child) override;
    sf::Vector2f OnMeasure(const BoxConstraints& constraints) override;

private:
    void ArrangeChildren();

    Thickness padding;
    std::unordered_map<const Widget*, Alignment> childAlignments;
    bool layoutInProgress = false;
};

class PaddingPanel final : public OverlayPanel {};

#endif
