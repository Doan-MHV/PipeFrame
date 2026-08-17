#include <PipeFrame/UI/Panel.h>

Panel::Panel() {
    background.setFillColor(sf::Color(28, 30, 38, 245));
    background.setOutlineColor(sf::Color(75, 82, 98));
    background.setOutlineThickness(1.0f);

    OnGeometryChanged();
}

void Panel::SetFillColor(sf::Color color) { background.setFillColor(color); }

void Panel::SetOutlineColor(sf::Color color) { background.setOutlineColor(color); }

void Panel::SetOutlineThickness(float thickness) { background.setOutlineThickness(thickness); }

sf::Color Panel::GetFillColor() const { return background.getFillColor(); }

sf::Color Panel::GetOutlineColor() const { return background.getOutlineColor(); }

float Panel::GetOutlineThickness() const { return background.getOutlineThickness(); }

void Panel::OnRender(sf::RenderTarget &target) const { target.draw(background); }

void Panel::OnGeometryChanged() {
    background.setPosition(GetScreenPosition());
    background.setSize(GetSize());
}
