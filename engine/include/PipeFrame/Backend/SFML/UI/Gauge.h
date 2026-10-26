#ifndef PIPEFRAME_UI_GAUGE_H
#define PIPEFRAME_UI_GAUGE_H

#include <PipeFrame/Backend/SFML/UI/Motion.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>
#include <PipeFrame/Backend/SFML/UI/Widget.h>

#include <SFML/Graphics/Vertex.hpp>
#include <vector>

class Gauge final : public Widget {
public:
    explicit Gauge(const UITheme& theme = UITheme::Dark());

    void SetRange(float minimum, float maximum);
    void SetValue(float value, bool animate = true);
    float GetValue() const;
    float GetVisualValue() const;
    void SetTrackColor(sf::Color color);
    void SetFillColor(sf::Color color);
    void SetThickness(float thickness);
    void SetSweep(float startDegrees, float sweepDegrees);
    void SetTransitionDuration(float seconds);
    void SetReducedMotion(bool reducedMotion) override;

protected:
    void OnRender(sf::RenderTarget& target) const override;
    void OnGeometryChanged() override;
    void OnOpacityChanged() override;
    void OnUpdate(float realDeltaSeconds) override;

private:
    void RebuildGeometry();
    float NormalizedVisualValue() const;

    std::vector<sf::Vertex> trackVertices;
    std::vector<sf::Vertex> fillVertices;
    pipeframe::ui::AnimatedFloat animatedValue;
    sf::Color trackColor;
    sf::Color fillColor;
    float minimum = 0.0f;
    float maximum = 1.0f;
    float value = 0.0f;
    float thickness = 6.0f;
    float startDegrees = -225.0f;
    float sweepDegrees = 270.0f;
    float transitionDuration = 0.0f;
};

#endif
