#include "DemoAgent.h"

#include <cmath>
#include <utility>

DemoAgent::DemoAgent(DemoAgentId newObjectId, std::string newName, sf::Vector2f initialPosition)
    : objectId(newObjectId), name(std::move(newName)), position(initialPosition) {
    marker.setRadius(48.0f);
    marker.setOrigin({48.0f, 48.0f});
    marker.setFillColor(sf::Color(232, 91, 116));

    body.setSize({220.0f, 120.0f});
    body.setOrigin({110.0f, 60.0f});
    body.setFillColor(sf::Color(55, 55, 55));

    SetPosition(position);
    SetRotation(0.0f);
    SetSelected(false);
}

DemoAgentId DemoAgent::GetId() const { return objectId; }

const std::string &DemoAgent::GetName() const { return name; }

void DemoAgent::SetPosition(sf::Vector2f newPosition) {
    position = newPosition;

    body.setPosition(position);
    marker.setPosition(position);
}

sf::Vector2f DemoAgent::GetPosition() const { return position; }

void DemoAgent::Move(sf::Vector2f offset) { SetPosition(position + offset); }

void DemoAgent::SetRotation(float degrees) {
    rotation = NormalizeDegrees(degrees);
    body.setRotation(sf::degrees(rotation));
}

float DemoAgent::GetRotation() const { return rotation; }

void DemoAgent::SetSelected(bool selected) {
    if (selected) {
        body.setOutlineColor(sf::Color(245, 179, 103));
        body.setOutlineThickness(4.0f);
    } else {
        body.setOutlineColor(sf::Color(95, 100, 110));
        body.setOutlineThickness(1.0f);
    }
}

void DemoAgent::SetSimulationPlaying(bool playing) {
    body.setFillColor(playing ? sf::Color(55, 55, 55) : sf::Color(45, 70, 110));
}

bool DemoAgent::Contains(sf::Vector2f worldPoint) const {
    const sf::Vector2f localPoint = body.getInverseTransform().transformPoint(worldPoint);

    return body.getLocalBounds().contains(localPoint);
}

void DemoAgent::Render(sf::RenderTarget &target) const {
    target.draw(body);
    target.draw(marker);
}

float DemoAgent::NormalizeDegrees(float degrees) {
    float normalized = std::fmod(degrees, 360.0f);

    if (normalized < 0.0f) {
        normalized += 360.0f;
    }

    return normalized;
}