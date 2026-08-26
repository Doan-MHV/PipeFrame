#ifndef PIPEFRAME_SCENE_TYPES_H
#define PIPEFRAME_SCENE_TYPES_H

#include <cstdint>
#include <optional>
#include <string>

#include <SFML/System/Vector2.hpp>

namespace pipeframe::editor {

using SceneObjectId = std::uint32_t;

enum class SceneObjectType : std::uint8_t { DemoAgent = 0, AgentPopulation = 1 };

struct AgentPopulationSettings {
    std::uint32_t agentCount = 1000;

    sf::Vector2f spawnAreaSize{1000.0f, 1000.0f};

    std::uint32_t randomSeed = 1;
};

struct SceneTransform {
    sf::Vector2f position{0.0f, 0.0f};
    float rotation = 0.0f;
};

struct SceneObjectData {
    SceneObjectId id = 0;
    std::string name;
    SceneObjectType type = SceneObjectType::DemoAgent;
    SceneTransform transform;

    std::optional<AgentPopulationSettings> population;
};

} // namespace pipeframe::editor

#endif