#include <PipeFrame/Backend/SFML/UI/RoundedRectangleShape.h>

#include <algorithm>
#include <cmath>
#include <numbers>

RoundedRectangleShape::RoundedRectangleShape(const sf::Vector2f newSize, const float newCornerRadius,
                                             const std::size_t newCornerPointCount)
    : size(newSize), cornerRadius(std::max(0.0f, newCornerRadius)),
      cornerPointCount(std::max<std::size_t>(2, newCornerPointCount)) {
    update();
}

void RoundedRectangleShape::setSize(const sf::Vector2f newSize) {
    const sf::Vector2f sanitized{std::max(0.0f, newSize.x), std::max(0.0f, newSize.y)};
    if (size == sanitized) {
        return;
    }
    size = sanitized;
    update();
}

sf::Vector2f RoundedRectangleShape::getSize() const { return size; }

void RoundedRectangleShape::setCornerRadius(const float newRadius) {
    const float sanitized = std::max(0.0f, newRadius);
    if (cornerRadius == sanitized) {
        return;
    }
    cornerRadius = sanitized;
    update();
}

float RoundedRectangleShape::getCornerRadius() const { return cornerRadius; }

void RoundedRectangleShape::setCornerPointCount(const std::size_t newCount) {
    const std::size_t sanitized = std::max<std::size_t>(2, newCount);
    if (cornerPointCount == sanitized) {
        return;
    }
    cornerPointCount = sanitized;
    update();
}

std::size_t RoundedRectangleShape::getCornerPointCount() const { return cornerPointCount; }

std::size_t RoundedRectangleShape::getPointCount() const {
    return cornerRadius <= 0.0f ? 4 : cornerPointCount * 4;
}

sf::Vector2f RoundedRectangleShape::getPoint(const std::size_t index) const {
    const float radius = std::min({cornerRadius, size.x * 0.5f, size.y * 0.5f});
    if (radius <= 0.0f) {
        switch (index % 4) {
        case 0:
            return {0.0f, 0.0f};
        case 1:
            return {size.x, 0.0f};
        case 2:
            return {size.x, size.y};
        default:
            return {0.0f, size.y};
        }
    }

    const std::size_t corner = index / cornerPointCount;
    const std::size_t cornerIndex = index % cornerPointCount;
    const float quarterTurn = std::numbers::pi_v<float> * 0.5f;
    const float angle = std::numbers::pi_v<float> + static_cast<float>(corner) * quarterTurn +
                        static_cast<float>(cornerIndex) * quarterTurn /
                            static_cast<float>(cornerPointCount - 1);

    sf::Vector2f center;
    switch (corner) {
    case 0:
        center = {radius, radius};
        break;
    case 1:
        center = {size.x - radius, radius};
        break;
    case 2:
        center = {size.x - radius, size.y - radius};
        break;
    default:
        center = {radius, size.y - radius};
        break;
    }

    return center + sf::Vector2f{std::cos(angle) * radius, std::sin(angle) * radius};
}
