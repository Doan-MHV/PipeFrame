#ifndef PIPEFRAME_UI_BADGE_H
#define PIPEFRAME_UI_BADGE_H

#include <string>

#include <SFML/Graphics/Font.hpp>

#include <PipeFrame/Backend/SFML/UI/Surface.h>

class Label;

class Badge final : public Surface {
  public:
    explicit Badge(const sf::Font &font, const std::string &text = "",
                   const UITheme &theme = UITheme::Dark());

    void SetText(const std::string &text);
    void SetTextColor(sf::Color color);

  protected:
    void OnGeometryChanged() override;

  private:
    Label *label = nullptr;
};

#endif
