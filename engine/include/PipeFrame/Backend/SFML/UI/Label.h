#ifndef PIPEFRAME_LABEL_H
#define PIPEFRAME_LABEL_H

#include <PipeFrame/Backend/SFML/UI/Widget.h>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <string>
#include <vector>

enum class LabelAlignment { Left, Center, Right };

class Label : public Widget {
public:
    explicit Label(const sf::Font& font);

    void SetText(const std::string& newText);
    std::string GetText() const;
    void SetWrap(bool enabled);
    void SetCharacterSize(unsigned int newCharacterSize);
    void SetColor(sf::Color newColor);
    void SetAlignment(LabelAlignment newAlignment);
    void SetHorizontalPadding(float newPadding);
    void SetRotation(float degrees);
    sf::Color GetColor() const;

protected:
    void OnRender(sf::RenderTarget& target) const override;
    void OnGeometryChanged() override;
    void OnOpacityChanged() override;
    sf::Vector2f OnMeasure(const BoxConstraints& constraints) override;

private:
    void RefreshTextPosition();
    sf::String WrappedText(float availableWidth) const;
    void RefreshOpacity();

    struct Measurement {
        float width;
        sf::Vector2f size;
    };
    std::vector<Measurement> measurements;
    sf::Text text;
    std::string content;
    bool wrap = false;
    bool textDirty = true;
    float cachedTextWidth = -1;
    sf::Color color{225, 230, 240};

    LabelAlignment alignment = LabelAlignment::Left;
    float horizontalPadding = 8.0f;
    float rotationDegrees = 0.0f;
};

#endif
