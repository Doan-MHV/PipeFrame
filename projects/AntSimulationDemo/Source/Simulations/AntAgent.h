#ifndef PIPEFRAME_ANT_AGENT_H
#define PIPEFRAME_ANT_AGENT_H

#include <vector>

#include <glm/glm.hpp>

enum class AntRole
{
    Follower,
    Explorer
};

enum class AntState
{
    ToFood,
    ToHomeWithFood,
    ToHomeNoFood
};

struct AntAgent
{
    int id = -1;
    glm::vec2 position = glm::vec2(0.0f);
    glm::vec2 velocity = glm::vec2(0.0f);
    glm::vec2 direction = glm::vec2(1.0f, 0.0f);
    glm::vec2 steeringDirection = glm::vec2(1.0f, 0.0f);
    glm::vec2 homePosition = glm::vec2(0.0f);
    glm::vec2 target = glm::vec2(0.0f);
    glm::vec2 lastMarkerPosition = glm::vec2(0.0f);
    AntRole role = AntRole::Follower;
    AntState state = AntState::ToFood;
    float distanceToTarget = 0.0f;
    float energy = 0.0f;
    float walkTime = 0.0f;
    float timeSinceMarker = 0.0f;
    float currentSpeed = 0.0f;
    float totalTravelDistance = 0.0f;
    int collectedFood = 0;
    bool blocked = false;
    double stuckTime = 0.0;
    double age = 0.0;
    float animationPhase = 0.0f;
};

#endif // PIPEFRAME_ANT_AGENT_H
