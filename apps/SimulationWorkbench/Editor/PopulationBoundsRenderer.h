#ifndef PIPEFRAME_POPULATION_BOUNDS_RENDERER_H
#define PIPEFRAME_POPULATION_BOUNDS_RENDERER_H

#include "SceneDocument.h"

#include <optional>

#include <SFML/Graphics/RenderTarget.hpp>

namespace pipeframe::editor {

class PopulationBoundsRenderer final {
  public:
    void Render(sf::RenderTarget &target, const SceneDocument &document,
                std::optional<SceneObjectId> selectedObjectId) const;
};

} // namespace pipeframe::editor

#endif