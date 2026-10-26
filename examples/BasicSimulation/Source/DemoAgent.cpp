#include "DemoAgent.h"

#include <cmath>
#include <utility>

namespace basic_simulation {

DemoAgent::DemoAgent(
    const pipeframe::SceneObjectId objectId,
    std::string name,
    const sf::Vector2f position)
    : objectId(objectId),
      name(std::move(name)),
      position(position) {

    marker.setRadius(48.0f);
    marker.setOrigin({48.0f, 48.0f});
    marker.setFillColor(
        sf::Color(232, 91, 116));

    body.setSize({220.0f, 120.0f});
    body.setOrigin({110.0f, 60.0f});
    body.setFillColor(
        sf::Color(45, 70, 110));

    SetPosition(position);
    SetRotation(0.0f);
    SetSelected(false);
}

pipeframe::SceneObjectId
DemoAgent::GetId() const {
    return objectId;
}

const std::string &
DemoAgent::GetName() const {
    return name;
}

void DemoAgent::SetPosition(
    const sf::Vector2f newPosition) {

    position = newPosition;

    body.setPosition(position);
    marker.setPosition(position);
}

sf::Vector2f
DemoAgent::GetPosition() const {
    return position;
}

void DemoAgent::SetRotation(
    const float degrees) {

    rotation = NormalizeDegrees(degrees);

    body.setRotation(
        sf::degrees(rotation));
}

float DemoAgent::GetRotation() const {
    return rotation;
}

void DemoAgent::SetAngularSpeed(
    const float degreesPerSecond) {

    angularSpeed = degreesPerSecond;
}

float DemoAgent::GetAngularSpeed() const {
    return angularSpeed;
}

void DemoAgent::SetSelected(
    const bool selected) {

    if (selected) {
        body.setOutlineColor(
            sf::Color(245, 179, 103));

        body.setOutlineThickness(4.0f);
    } else {
        body.setOutlineColor(
            sf::Color(95, 100, 110));

        body.setOutlineThickness(1.0f);
    }
}

void DemoAgent::SetSimulationPlaying(
    const bool playing) {

    body.setFillColor(
        playing
            ? sf::Color(55, 55, 55)
            : sf::Color(45, 70, 110));
}

bool DemoAgent::Contains(
    const sf::Vector2f worldPoint) const {

    const sf::Vector2f localPoint =
        body.getInverseTransform()
            .transformPoint(worldPoint);

    return body.getLocalBounds()
        .contains(localPoint);
}

void DemoAgent::FixedUpdate(
    const float fixedDeltaTime) {

    SetRotation(
        rotation +
        angularSpeed * fixedDeltaTime);
}

void DemoAgent::Render(
    sf::RenderTarget &target) const {

    target.draw(body);
    target.draw(marker);
}

float DemoAgent::NormalizeDegrees(
    const float degrees) {

    float normalized =
        std::fmod(degrees, 360.0f);

    if (normalized < 0.0f) {
        normalized += 360.0f;
    }

    return normalized;
}

} // namespace basic_simulation