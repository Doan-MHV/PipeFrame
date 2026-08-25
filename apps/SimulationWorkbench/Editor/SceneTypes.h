#ifndef PIPEFRAME_SCENE_TYPES_H
#define PIPEFRAME_SCENE_TYPES_H

#include <cstdint>
#include <string>

#include <SFML/System/Vector2.hpp>

namespace pipeframe::editor {

using SceneObjectId = std::uint32_t;

enum class SceneObjectType { DemoAgent };

struct SceneTransform {
    sf::Vector2f position{0.0f, 0.0f};
    float rotation = 0.0f;
};

struct SceneObjectData {
    SceneObjectId id = 0;
    std::string name;
    SceneObjectType type = SceneObjectType::DemoAgent;
    SceneTransform transform;
};

} // namespace pipeframe::editor

#endif