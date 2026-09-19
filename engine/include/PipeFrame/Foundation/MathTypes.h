#ifndef PIPEFRAME_FOUNDATION_MATH_TYPES_H
#define PIPEFRAME_FOUNDATION_MATH_TYPES_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace pipeframe {

template <typename T>
struct Vector2 {
    T x{};
    T y{};

    template <typename U>
    constexpr explicit operator Vector2<U>() const {
        return {static_cast<U>(x), static_cast<U>(y)};
    }

    constexpr Vector2& operator+=(const Vector2 other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    constexpr Vector2& operator-=(const Vector2 other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    constexpr Vector2& operator*=(const T scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    constexpr Vector2& operator/=(const T scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }
    constexpr bool operator==(const Vector2&) const = default;
};

template <typename T>
constexpr Vector2<T> operator+(Vector2<T> a, const Vector2<T> b) {
    return a += b;
}
template <typename T>
constexpr Vector2<T> operator-(Vector2<T> a, const Vector2<T> b) {
    return a -= b;
}
template <typename T>
constexpr Vector2<T> operator-(const Vector2<T> value) {
    return {-value.x, -value.y};
}
template <typename T>
constexpr Vector2<T> operator*(Vector2<T> value, const T scalar) {
    return value *= scalar;
}
template <typename T>
constexpr Vector2<T> operator*(const T scalar, Vector2<T> value) {
    return value *= scalar;
}
template <typename T>
constexpr Vector2<T> operator/(Vector2<T> value, const T scalar) {
    return value /= scalar;
}

using Vector2f = Vector2<float>;
using Vector2i = Vector2<std::int32_t>;
using Vector2u = Vector2<std::uint32_t>;

template <typename T>
struct Rectangle {
    Vector2<T> position{};
    Vector2<T> size{};

    [[nodiscard]] constexpr bool Contains(const Vector2<T> point) const {
        return point.x >= position.x && point.y >= position.y && point.x < position.x + size.x &&
               point.y < position.y + size.y;
    }

    constexpr bool operator==(const Rectangle&) const = default;
};

using Rectanglef = Rectangle<float>;
using Rectanglei = Rectangle<std::int32_t>;

struct Transform2D {
    Vector2f position{};
    float rotationRadians{0.0f};
    Vector2f scale{1.0f, 1.0f};
    constexpr bool operator==(const Transform2D&) const = default;
};

struct Color {
    union {
        std::uint8_t red;
        std::uint8_t r;
    };
    union {
        std::uint8_t green;
        std::uint8_t g;
    };
    union {
        std::uint8_t blue;
        std::uint8_t b;
    };
    union {
        std::uint8_t alpha;
        std::uint8_t a;
    };
    constexpr Color(std::uint8_t red = 0, std::uint8_t green = 0, std::uint8_t blue = 0, std::uint8_t alpha = 255)
        : red(red), green(green), blue(blue), alpha(alpha) {}
    constexpr bool operator==(const Color& other) const {
        return red == other.red && green == other.green && blue == other.blue && alpha == other.alpha;
    }
    static const Color White, Black, Red, Green, Blue, Transparent;
};

inline constexpr Color Color::White{255, 255, 255, 255};
inline constexpr Color Color::Black{0, 0, 0, 255};
inline constexpr Color Color::Red{255, 0, 0, 255};
inline constexpr Color Color::Green{0, 255, 0, 255};
inline constexpr Color Color::Blue{0, 0, 255, 255};
inline constexpr Color Color::Transparent{0, 0, 0, 0};

class Angle {
public:
    static constexpr Angle FromRadians(const float value) { return Angle(value); }
    static constexpr Angle FromDegrees(const float value) { return Angle(value * Pi / 180.0f); }
    [[nodiscard]] constexpr float Radians() const { return radians; }
    [[nodiscard]] constexpr float Degrees() const { return radians * 180.0f / Pi; }
    constexpr bool operator==(const Angle&) const = default;

private:
    explicit constexpr Angle(const float value) : radians(value) {}
    static constexpr float Pi = 3.14159265358979323846f;
    float radians{};
};

class TimeSpan {
public:
    static constexpr TimeSpan FromSeconds(const double value) { return TimeSpan(value); }
    static constexpr TimeSpan FromMilliseconds(const double value) { return TimeSpan(value / 1000.0); }
    [[nodiscard]] constexpr double Seconds() const { return seconds; }
    [[nodiscard]] constexpr double Milliseconds() const { return seconds * 1000.0; }
    constexpr bool operator==(const TimeSpan&) const = default;

private:
    explicit constexpr TimeSpan(const double value) : seconds(value) {}
    double seconds{};
};

template <typename T>
[[nodiscard]] inline T LengthSquared(const Vector2<T> value) {
    return value.x * value.x + value.y * value.y;
}

[[nodiscard]] inline float Length(const Vector2f value) {
    return std::sqrt(LengthSquared(value));
}

}  // namespace pipeframe

#endif
