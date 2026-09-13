#ifndef PIPEFRAME_UI_COLLAPSIBLE_PANEL_H
#define PIPEFRAME_UI_COLLAPSIBLE_PANEL_H

#include <PipeFrame/Backend/SFML/UI/ScrollPanel.h>
#include <PipeFrame/Backend/SFML/UI/StackPanel.h>
#include <PipeFrame/Backend/SFML/UI/TextButton.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

// Retained body with an animated viewport; overflow remains scrollable when expanded.
class CollapsiblePanel final : public Column {
  public:
    explicit CollapsiblePanel(const sf::Font &font, const UITheme &theme = UITheme::Dark());
    TextButton &GetHeader();
    Column &GetContent();
    void SetCaption(std::string caption);
    void SetExpandedHeight(float height);
    void SetExpanded(bool expanded, bool animate = true);
    bool IsExpanded() const;
    void SetReducedMotion(bool reduced) override;

  protected:
    void OnUpdate(float realDeltaSeconds) override;
    void OnGeometryChanged() override;

  private:
    void ApplyHeight();
    TextButton *header = nullptr;
    ScrollPanel *viewport = nullptr;
    Column *content = nullptr;
    pipeframe::ui::AnimatedFloat height;
    std::string caption = "Details";
    float expandedHeight = 144;
    float duration;
    bool expanded = false;
};

#endif
