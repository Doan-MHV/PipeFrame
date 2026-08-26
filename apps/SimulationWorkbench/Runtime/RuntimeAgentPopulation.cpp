#include "RuntimeAgentPopulation.h"

#include <random>

namespace pipeframe::runtime {

bool RuntimeAgentPopulation::Initialize(const editor::SceneObjectData &sourceObject) {

    Clear();

    if (sourceObject.type != editor::SceneObjectType::AgentPopulation || !sourceObject.population.has_value()) {
        return false;
    }

    sourceObjectId = sourceObject.id;
    transform = sourceObject.transform;

    const editor::AgentPopulationSettings &settings = *sourceObject.population;

    halfWidth = settings.spawnAreaSize.x * 0.5f;
    halfHeight = settings.spawnAreaSize.y * 0.5f;

    const std::size_t count = static_cast<std::size_t>(settings.agentCount);

    positionX.resize(count);
    positionY.resize(count);
    velocityX.resize(count);
    velocityY.resize(count);

    std::mt19937 randomGenerator{settings.randomSeed};

    std::uniform_real_distribution<float> spawnX{
        -halfWidth,
        halfWidth,
    };

    std::uniform_real_distribution<float> spawnY{
        -halfHeight,
        halfHeight,
    };

    std::uniform_real_distribution<float> velocity{
        -120.0f,
        120.0f,
    };

    for (std::size_t index = 0; index < count; ++index) {
        positionX[index] = spawnX(randomGenerator);
        positionY[index] = spawnY(randomGenerator);

        velocityX[index] = velocity(randomGenerator);
        velocityY[index] = velocity(randomGenerator);
    }

    return true;
}

void RuntimeAgentPopulation::Clear() {
    sourceObjectId = 0;
    transform = {};

    halfWidth = 0.0f;
    halfHeight = 0.0f;

    positionX.clear();
    positionY.clear();
    velocityX.clear();
    velocityY.clear();
}

void RuntimeAgentPopulation::Update(float fixedDeltaTime) {
    const std::size_t count = positionX.size();

    for (std::size_t index = 0; index < count; ++index) {
        positionX[index] += velocityX[index] * fixedDeltaTime;
        positionY[index] += velocityY[index] * fixedDeltaTime;

        if (positionX[index] < -halfWidth) {
            positionX[index] = -halfWidth;
            velocityX[index] = -velocityX[index];
        } else if (positionX[index] > halfWidth) {
            positionX[index] = halfWidth;
            velocityX[index] = -velocityX[index];
        }

        if (positionY[index] < -halfHeight) {
            positionY[index] = -halfHeight;
            velocityY[index] = -velocityY[index];
        } else if (positionY[index] > halfHeight) {
            positionY[index] = halfHeight;
            velocityY[index] = -velocityY[index];
        }
    }
}

std::size_t RuntimeAgentPopulation::GetCount() const { return positionX.size(); }

editor::SceneObjectId RuntimeAgentPopulation::GetSourceObjectId() const { return sourceObjectId; }

const editor::SceneTransform &RuntimeAgentPopulation::GetTransform() const { return transform; }

const std::vector<float> &RuntimeAgentPopulation::GetPositionX() const { return positionX; }

const std::vector<float> &RuntimeAgentPopulation::GetPositionY() const { return positionY; }

} // namespace pipeframe::runtime