#ifndef PIPEFRAME_TEXT_BUTTON_H
#define PIPEFRAME_TEXT_BUTTON_H

#include <string>

#include <SFML/Graphics/Font.hpp>

#include <PipeFrame/UI/Button.h>

class Label;

class TextButton : public Button {
  public:
    explicit TextButton(const sf::Font &font);

    void SetText(const std::string &text);
    void SetTextCharacterSize(unsigned int characterSize);

  protected:
    void OnGeometryChanged() override;

  private:
    Label *label = nullptr;
};

#endif