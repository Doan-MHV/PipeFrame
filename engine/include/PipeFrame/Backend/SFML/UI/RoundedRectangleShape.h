#ifndef PIPEFRAME_UI_ROUNDED_RECTANGLE_SHAPE_H
#define PIPEFRAME_UI_ROUNDED_RECTANGLE_SHAPE_H

#include <cstddef>

#include <SFML/Graphics/Shape.hpp>
#include <SFML/System/Vector2.hpp>

class RoundedRectangleShape final : public sf::Shape {
  public:
    explicit RoundedRectangleShape(sf::Vector2f size = {}, float cornerRadius = 0.0f,
                                   std::size_t cornerPointCount = 6);

    void setSize(sf::Vector2f newSize);
    sf::Vector2f getSize() const;

    void setCornerRadius(float newRadius);
    float getCornerRadius() const;

    void setCornerPointCount(std::size_t newCount);
    std::size_t getCornerPointCount() const;

    std::size_t getPointCount() const override;
    sf::Vector2f getPoint(std::size_t index) const override;

  private:
    sf::Vector2f size;
    float cornerRadius = 0.0f;
    std::size_t cornerPointCount = 6;
};

#endif
