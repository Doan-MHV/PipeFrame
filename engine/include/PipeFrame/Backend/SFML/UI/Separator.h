#ifndef PIPEFRAME_UI_SEPARATOR_H
#define PIPEFRAME_UI_SEPARATOR_H

#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

enum class SeparatorOrientation {
    Horizontal,
    Vertical,
};

class Separator final : public Panel {
public:
    explicit Separator(const SeparatorOrientation orientation = SeparatorOrientation::Horizontal,
                       const UITheme& theme = UITheme::Dark()) {
        SetFillColor(theme.subtleBorder);
        SetOutlineThickness(0.0f);
        SetHitTestVisible(false);
        SetSize(orientation == SeparatorOrientation::Horizontal ? sf::Vector2f{1.0f, 1.0f} : sf::Vector2f{1.0f, 1.0f});
        SetSizePolicy(orientation == SeparatorOrientation::Horizontal ? SizePolicy::Stretch : SizePolicy::Fixed,
                      orientation == SeparatorOrientation::Horizontal ? SizePolicy::Fixed : SizePolicy::Stretch);
    }
};

#endif
