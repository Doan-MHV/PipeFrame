#ifndef PIPEFRAME_SCROLL_PANEL_H
#define PIPEFRAME_SCROLL_PANEL_H

#include <PipeFrame/Backend/SFML/UI/Panel.h>

class ScrollPanel : public Panel {
public:
    void SetContent(Widget& newContent);
    Widget* GetContent();
    const Widget* GetContent() const;

    void SetScrollOffset(float newOffset);
    float GetScrollOffset() const;

    void SetWheelStep(float newWheelStep);
    float GetWheelStep() const;

    float GetMaximumScrollOffset() const;

    void ScrollBy(float delta);

    void Render(sf::RenderTarget& target) const override;

protected:
    bool OnEvent(const sf::Event& event) override;

    void OnGeometryChanged() override;

    void OnChildGeometryChanged(Widget& child) override;
    void OnChildRemoved(Widget& child) override;

    bool ClipsChildren() const override;

private:
    void RefreshContentPosition();

    Widget* content = nullptr;

    float scrollOffset = 0.0f;
    float wheelStep = 36.0f;
};

#endif