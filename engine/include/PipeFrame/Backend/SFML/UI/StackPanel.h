#ifndef PIPEFRAME_STACK_PANEL_H
#define PIPEFRAME_STACK_PANEL_H

#include <PipeFrame/Backend/SFML/UI/Panel.h>

#include <unordered_map>

enum class StackOrientation {
    Vertical,
    Horizontal,
};

enum class MainAxisAlignment {
    Start,
    Center,
    End,
    SpaceBetween,
};

enum class CrossAxisAlignment {
    Start,
    Center,
    End,
    Stretch,
};

class StackPanel : public Panel {
  public:
    void SetOrientation(StackOrientation newOrientation);

    StackOrientation GetOrientation() const;

    void SetPadding(Thickness newPadding);
    Thickness GetPadding() const;

    void SetSpacing(float newSpacing);
    float GetSpacing() const;

    void SetMainAxisAlignment(MainAxisAlignment newAlignment);

    MainAxisAlignment GetMainAxisAlignment() const;

    void SetCrossAxisAlignment(CrossAxisAlignment newAlignment);

    CrossAxisAlignment GetCrossAxisAlignment() const;

    void SetChildFlex(Widget &child, float flex);

    float GetChildFlex(const Widget &child) const;

    void RefreshLayout();

  protected:
    void OnGeometryChanged() override;

    void OnChildGeometryChanged(Widget &child) override;
    void OnChildRemoved(Widget &child) override;

    sf::Vector2f OnMeasure(const BoxConstraints &constraints) override;

  private:
    void LayoutVertical();
    void LayoutHorizontal();

    float CalculateLeadingOffset(float unusedSpace, std::size_t visibleChildCount) const;

    float CalculateSpacing(float unusedSpace, std::size_t visibleChildCount) const;

    StackOrientation orientation = StackOrientation::Vertical;

    MainAxisAlignment mainAxisAlignment = MainAxisAlignment::Start;

    CrossAxisAlignment crossAxisAlignment = CrossAxisAlignment::Stretch;

    Thickness padding;
    float spacing = 0.0f;

    std::unordered_map<const Widget *, float> childFlex;

    bool layoutInProgress = false;
};

class Row : public StackPanel {
  public:
    Row() { SetOrientation(StackOrientation::Horizontal); }
};

class Column : public StackPanel {};

#endif
