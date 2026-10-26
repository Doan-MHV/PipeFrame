#ifndef PIPEFRAME_TEXT_BUTTON_H
#define PIPEFRAME_TEXT_BUTTON_H

#include <PipeFrame/Backend/SFML/UI/Button.h>

#include <SFML/Graphics/Font.hpp>
#include <string>

class Label;

class TextButton : public Button {
public:
    explicit TextButton(const sf::Font& font);

    void SetText(const std::string& text);
    std::string GetText() const;
    void SetTextCharacterSize(unsigned int characterSize);
    void SetTextWrap(bool wrap);
    void SetTextLeading(bool leading);

protected:
    void OnGeometryChanged() override;
    sf::Vector2f OnMeasure(const BoxConstraints& constraints) override;

private:
    Label* label = nullptr;
    bool wrappedText{false};
};

#endif
