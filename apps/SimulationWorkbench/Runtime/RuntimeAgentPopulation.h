#ifndef PIPEFRAME_RUNTIME_AGENT_POPULATION_H
#define PIPEFRAME_RUNTIME_AGENT_POPULATION_H

#include "../Editor/SceneTypes.h"

#include <cstddef>
#include <vector>

namespace pipeframe::runtime {

class RuntimeAgentPopulation final {
  public:
    bool Initialize(const editor::SceneObjectData &sourceObject);

    void Clear();
    void Update(float fixedDeltaTime);

    std::size_t GetCount() const;

    editor::SceneObjectId GetSourceObjectId() const;
    const editor::SceneTransform &GetTransform() const;

    const std::vector<float> &GetPositionX() const;
    const std::vector<float> &GetPositionY() const;

  private:
    editor::SceneObjectId sourceObjectId = 0;
    editor::SceneTransform transform;

    float halfWidth = 0.0f;
    float halfHeight = 0.0f;

    // Structure-of-arrays storage.
    std::vector<float> positionX;
    std::vector<float> positionY;
    std::vector<float> velocityX;
    std::vector<float> velocityY;
};

} // namespace pipeframe::runtime

#endif