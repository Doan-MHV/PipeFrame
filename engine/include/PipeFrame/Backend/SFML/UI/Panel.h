#ifndef PIPEFRAME_PANEL_H
#define PIPEFRAME_PANEL_H

#include <PipeFrame/Backend/SFML/UI/RoundedRectangleShape.h>
#include <PipeFrame/Backend/SFML/UI/Widget.h>

#include <SFML/Graphics/Color.hpp>

class Panel : public Widget {
public:
    Panel();

    void SetFillColor(sf::Color color);
    void SetOutlineColor(sf::Color color);
    void SetOutlineThickness(float thickness);
    void SetCornerRadius(float radius);
    void SetShadowColor(sf::Color color);
    void SetShadowOffset(sf::Vector2f offset);

    sf::Color GetFillColor() const;
    sf::Color GetOutlineColor() const;
    float GetOutlineThickness() const;
    float GetCornerRadius() const;
    sf::Color GetShadowColor() const;
    sf::Vector2f GetShadowOffset() const;

protected:
    void OnRender(sf::RenderTarget& target) const override;
    void OnGeometryChanged() override;
    void OnOpacityChanged() override;

private:
    void RefreshOpacity();

    RoundedRectangleShape background;
    RoundedRectangleShape shadow;
    sf::Color fillColor{28, 30, 38, 245};
    sf::Color outlineColor{75, 82, 98};
    sf::Color shadowColor = sf::Color::Transparent;
    sf::Vector2f shadowOffset{0.0f, 0.0f};
};

#endif
