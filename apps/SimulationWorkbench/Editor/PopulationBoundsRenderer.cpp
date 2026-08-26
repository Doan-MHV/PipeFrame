#include "PopulationBoundsRenderer.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

namespace pipeframe::editor {

void PopulationBoundsRenderer::Render(sf::RenderTarget &target, const SceneDocument &document,
                                      std::optional<SceneObjectId> selectedObjectId) const {

    for (const SceneObjectData &object : document.GetObjects()) {
        if (object.type != SceneObjectType::AgentPopulation || !object.population.has_value()) {
            continue;
        }

        const AgentPopulationSettings &settings = *object.population;
        const bool selected = selectedObjectId.has_value() && *selectedObjectId == object.id;

        sf::RectangleShape spawnBounds;

        spawnBounds.setSize(settings.spawnAreaSize);
        spawnBounds.setOrigin({
            settings.spawnAreaSize.x * 0.5f,
            settings.spawnAreaSize.y * 0.5f,
        });

        spawnBounds.setPosition(object.transform.position);
        spawnBounds.setRotation(sf::degrees(object.transform.rotation));

        spawnBounds.setFillColor(selected ? sf::Color(60, 110, 175, 24) : sf::Color(60, 90, 130, 12));

        spawnBounds.setOutlineColor(selected ? sf::Color(245, 179, 103) : sf::Color(85, 105, 135));

        spawnBounds.setOutlineThickness(selected ? 4.0f : 2.0f);

        target.draw(spawnBounds);

        // The center marker remains visible even when the population
        // bounds extend far beyond the viewport.
        sf::CircleShape centerMarker{10.0f};

        centerMarker.setOrigin({10.0f, 10.0f});
        centerMarker.setPosition(object.transform.position);
        centerMarker.setFillColor(sf::Color::Transparent);

        centerMarker.setOutlineColor(selected ? sf::Color(245, 179, 103) : sf::Color(100, 130, 170));

        centerMarker.setOutlineThickness(2.0f);

        target.draw(centerMarker);
    }
}

} // namespace pipeframe::editor