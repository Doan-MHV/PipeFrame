#ifndef PIPEFRAME_LABEL_H
#define PIPEFRAME_LABEL_H

#include <string>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>

#include <PipeFrame/UI/Widget.h>

enum class LabelAlignment { Left, Center, Right };

class Label : public Widget {
  public:
    explicit Label(const sf::Font &font);

    void SetText(const std::string &newText);
    void SetCharacterSize(unsigned int newCharacterSize);
    void SetColor(sf::Color newColor);
    void SetAlignment(LabelAlignment newAlignment);
    void SetHorizontalPadding(float newPadding);

  protected:
    void OnRender(sf::RenderTarget &target) const override;
    void OnGeometryChanged() override;

  private:
    void RefreshTextPosition();

    sf::Text text;

    LabelAlignment alignment = LabelAlignment::Left;
    float horizontalPadding = 8.0f;
};

#endif