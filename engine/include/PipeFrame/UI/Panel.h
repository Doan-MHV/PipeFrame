#ifndef PIPEFRAME_PANEL_H
#define PIPEFRAME_PANEL_H

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <PipeFrame/UI/Widget.h>

class Panel : public Widget {
  public:
    Panel();

    void SetFillColor(sf::Color color);
    void SetOutlineColor(sf::Color color);
    void SetOutlineThickness(float thickness);

    sf::Color GetFillColor() const;
    sf::Color GetOutlineColor() const;
    float GetOutlineThickness() const;

  protected:
    void OnRender(sf::RenderTarget &target) const override;
    void OnGeometryChanged() override;

  private:
    sf::RectangleShape background;
};

#endif