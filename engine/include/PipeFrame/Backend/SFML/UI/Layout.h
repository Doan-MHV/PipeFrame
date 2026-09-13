#ifndef PIPEFRAME_UI_LAYOUT_H
#define PIPEFRAME_UI_LAYOUT_H

#include <algorithm>
#include <cmath>
#include <limits>

#include <SFML/System/Vector2.hpp>

enum class SizePolicy {
    Fixed,
    FitContent,
    Stretch,
};

struct Thickness {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    Thickness() = default;
    explicit Thickness(const float uniform) : left(uniform), top(uniform), right(uniform), bottom(uniform) {}
    Thickness(const float newLeft, const float newTop, const float newRight, const float newBottom)
        : left(newLeft), top(newTop), right(newRight), bottom(newBottom) {}
};

enum class HorizontalAlignment {
    Start,
    Center,
    End,
    Stretch,
};

enum class VerticalAlignment {
    Start,
    Center,
    End,
    Stretch,
};

struct Alignment {
    HorizontalAlignment horizontal = HorizontalAlignment::Stretch;
    VerticalAlignment vertical = VerticalAlignment::Stretch;
};

struct BoxConstraints {
    sf::Vector2f minimum{0.0f, 0.0f};
    sf::Vector2f maximum{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};

    static BoxConstraints Tight(const sf::Vector2f size) { return {size, size}; }
    static BoxConstraints Unbounded() { return {}; }

    [[nodiscard]] bool HasBoundedWidth() const { return std::isfinite(maximum.x); }
    [[nodiscard]] bool HasBoundedHeight() const { return std::isfinite(maximum.y); }

    [[nodiscard]] sf::Vector2f Constrain(const sf::Vector2f size) const {
        const float minimumWidth = std::max(0.0f, minimum.x);
        const float minimumHeight = std::max(0.0f, minimum.y);
        const float maximumWidth = std::max(minimumWidth, maximum.x);
        const float maximumHeight = std::max(minimumHeight, maximum.y);

        return {
            std::clamp(std::max(0.0f, size.x), minimumWidth, maximumWidth),
            std::clamp(std::max(0.0f, size.y), minimumHeight, maximumHeight),
        };
    }
};

#endif
