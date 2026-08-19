#ifndef PIPEFRAME_STACK_PANEL_H
#define PIPEFRAME_STACK_PANEL_H

#include <PipeFrame/UI/Panel.h>

enum class StackOrientation { Vertical, Horizontal };

struct Thickness {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    Thickness() = default;

    explicit Thickness(float uniform) : left(uniform), top(uniform), right(uniform), bottom(uniform) {}

    Thickness(float newLeft, float newTop, float newRight, float newBottom)
        : left(newLeft), top(newTop), right(newRight), bottom(newBottom) {}
};

class StackPanel : public Panel {
  public:
    void SetOrientation(StackOrientation newOrientation);
    StackOrientation GetOrientation() const;

    void SetPadding(Thickness newPadding);
    Thickness GetPadding() const;

    void SetSpacing(float newSpacing);
    float GetSpacing() const;

    void RefreshLayout();

  protected:
    void OnGeometryChanged() override;
    void OnChildGeometryChanged(Widget &child) override;

  private:
    void LayoutVertical();
    void LayoutHorizontal();

    StackOrientation orientation = StackOrientation::Vertical;
    Thickness padding;
    float spacing = 0.0f;

    bool layoutInProgress = false;
};

#endif