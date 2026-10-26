#ifndef PIPEFRAME_UI_MOTION_H
#define PIPEFRAME_UI_MOTION_H

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace pipeframe::ui {

inline float EaseOutCubic(const float progress) {
    const float clamped = std::clamp(progress, 0.0f, 1.0f);
    const float inverse = 1.0f - clamped;
    return 1.0f - inverse * inverse * inverse;
}

inline sf::Color LerpColor(const sf::Color from, const sf::Color to, const float progress) {
    const float eased = EaseOutCubic(progress);
    const auto channel = [eased](const std::uint8_t start, const std::uint8_t end) {
        return static_cast<std::uint8_t>(std::clamp(
            std::lround(static_cast<float>(start) + (static_cast<float>(end) - static_cast<float>(start)) * eased), 0l,
            255l));
    };
    return {channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b), channel(from.a, to.a)};
}

class AnimatedFloat final {
public:
    explicit AnimatedFloat(const float initial = 0.0f) : value(initial), start(initial), target(initial) {}

    void SetTarget(const float newTarget, const bool snap = false) {
        if (newTarget == target && !snap) {
            return;
        }
        start = value;
        target = newTarget;
        elapsed = 0.0f;
        if (snap) {
            value = target;
        }
    }

    bool Update(const float realDeltaSeconds, const float durationSeconds) {
        if (value == target) {
            return false;
        }
        if (durationSeconds <= 0.0f) {
            value = target;
            return true;
        }
        elapsed = std::min(durationSeconds, elapsed + std::max(0.0f, realDeltaSeconds));
        const float eased = EaseOutCubic(elapsed / durationSeconds);
        value = start + (target - start) * eased;
        if (elapsed >= durationSeconds) {
            value = target;
        }
        return true;
    }

    float Get() const { return value; }
    float GetTarget() const { return target; }

private:
    float value;
    float start;
    float target;
    float elapsed = 0.0f;
};

class AnimatedVector2 final {
public:
    explicit AnimatedVector2(const sf::Vector2f initial = {0.0f, 0.0f}) : x(initial.x), y(initial.y) {}

    void SetTarget(const sf::Vector2f target, const bool snap = false) {
        x.SetTarget(target.x, snap);
        y.SetTarget(target.y, snap);
    }

    bool Update(const float realDeltaSeconds, const float durationSeconds) {
        const bool xChanged = x.Update(realDeltaSeconds, durationSeconds);
        const bool yChanged = y.Update(realDeltaSeconds, durationSeconds);
        return xChanged || yChanged;
    }

    sf::Vector2f Get() const { return {x.Get(), y.Get()}; }
    sf::Vector2f GetTarget() const { return {x.GetTarget(), y.GetTarget()}; }

private:
    AnimatedFloat x;
    AnimatedFloat y;
};

class AnimatedColor final {
public:
    explicit AnimatedColor(const sf::Color initial = sf::Color::Transparent)
        : value(initial), start(initial), target(initial) {}

    void SetTarget(const sf::Color newTarget, const bool snap = false) {
        if (newTarget == target && !snap) {
            return;
        }
        start = value;
        target = newTarget;
        elapsed = 0.0f;
        if (snap) {
            value = target;
        }
    }

    bool Update(const float realDeltaSeconds, const float durationSeconds) {
        if (value == target) {
            return false;
        }
        if (durationSeconds <= 0.0f) {
            value = target;
            return true;
        }
        elapsed = std::min(durationSeconds, elapsed + std::max(0.0f, realDeltaSeconds));
        value = LerpColor(start, target, elapsed / durationSeconds);
        return true;
    }

    const sf::Color& Get() const { return value; }
    const sf::Color& GetTarget() const { return target; }

private:
    sf::Color value;
    sf::Color start;
    sf::Color target;
    float elapsed = 0.0f;
};

}  // namespace pipeframe::ui

#endif
