#ifndef PIPEFRAME_UI_TOOLTIP_H
#define PIPEFRAME_UI_TOOLTIP_H

#include <string>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/Surface.h>

class Tooltip final : public Surface {
  public:
    explicit Tooltip(const sf::Font &font, const UITheme &theme = UITheme::Dark());

    void SetText(const std::string &text);
    void SetShowDelay(float seconds);
    void ShowAt(sf::Vector2f anchorScreenPosition, const sf::FloatRect &viewport);
    void Hide();

    bool IsOpening() const;
    bool IsClosing() const;

  protected:
    void OnGeometryChanged() override;
    void OnUpdate(float realDeltaSeconds) override;

  private:
    void BeginOpening();

    Label &label;
    UITheme theme;
    float showDelay = 0.35f;
    float delayRemaining = 0.0f;
    bool opening = false;
    bool closing = false;
};

#endif
