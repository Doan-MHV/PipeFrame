#include "World/Physics/AntAvoidanceSystem.h"

#include <algorithm>
#include <cmath>

namespace ant_simulation {

AntAvoidanceSystem::AntAvoidanceSystem(
    AntQuery &newAntQuery,
    AntBodySystem &newPhysicsWorld,
    const AntConfiguration &newConfiguration
)
    : antStore(newAntQuery),
      physicsWorld(newPhysicsWorld),
      configuration(newConfiguration) {
    collisionGrid.Initialize(
        {{0,0},configuration.GetWorldSizeFloat()},
        1.0f);
}

std::size_t AntAvoidanceSystem::Update(
    const float deltaTime
) {
    if (deltaTime <= 0.0f) {
        futureCollisions.clear();
        return 0;
    }

    FindFutureCollisions();
    SolveFutureCollisions(deltaTime);

    return futureCollisions.size();
}

const std::vector<
    AntAvoidanceSystem::FutureCollision> &
AntAvoidanceSystem::
GetFutureCollisions() const {
    return futureCollisions;
}

std::optional<float>
AntAvoidanceSystem::
CalculateTimeToCollision(
    const AntPhysicsBody &first,
    const AntPhysicsBody &second,
    const float combinedRadius
) {
    const pipeframe::Vector2f positionDifference =
        second.position -
        first.position;

    const pipeframe::Vector2f velocityDifference =
        second.velocity -
        first.velocity;

    const float radiusSquared =
        combinedRadius *
        combinedRadius;

    const float c =
        positionDifference.x *
            positionDifference.x +
        positionDifference.y *
            positionDifference.y -
        radiusSquared;

    if (c <= 0.0f) {
        return 0.0f;
    }

    const float a =
        velocityDifference.x *
            velocityDifference.x +
        velocityDifference.y *
            velocityDifference.y;

    constexpr float Epsilon{
        0.000000000001f
    };

    if (a < Epsilon) {
        return std::nullopt;
    }

    const float b =
        positionDifference.x *
            velocityDifference.x +
        positionDifference.y *
            velocityDifference.y;

    const float discriminant =
        b * b -
        a * c;

    if (discriminant < 0.0f) {
        return std::nullopt;
    }

    const float collisionTime =
        (-b -
         std::sqrt(discriminant)) /
        a;

    if (collisionTime < 0.0f) {
        return std::nullopt;
    }

    return collisionTime;
}

void AntAvoidanceSystem::
FindFutureCollisions() {
    futureCollisions.clear();

    physicsWorld
        .SynchronizeAntPositions();

    gridEntries.clear();
    gridEntries.reserve(antStore.GetAnts().size());
    // Preserve AntQuery insertion order, including after body compaction, so
    // the solver visits valid candidates in the same order as the ID grid.
    for(const auto &ant:antStore.GetAnts())
        if(auto *body=physicsWorld.FindAntBody(ant.GetId()))
            gridEntries.push_back({body,ant.GetPosition()});
    collisionGrid.Rebuild(gridEntries);

    const std::span<AntPhysicsBody> bodies =
        physicsWorld.GetBodies();

    if (bodies.empty()) {
        return;
    }

    const std::size_t sliceBegin =
        bodies.size() *
        currentSlice /
        SliceCount;

    const std::size_t sliceEnd =
        bodies.size() *
        (currentSlice + 1) /
        SliceCount;

    currentSlice =
        (currentSlice + 1) %
        SliceCount;

    float maximumSpeed=configuration.antSpeed;
    for (const auto &body : bodies) maximumSpeed=std::max(maximumSpeed,std::sqrt(body.velocity.x*body.velocity.x+body.velocity.y*body.velocity.y));
    const float searchRadius =
        maximumSpeed *
        MaximumTimeToCollision *
        2.0f;

    for (std::size_t index = sliceBegin;
         index < sliceEnd;
         ++index) {
        AntPhysicsBody &first =
            bodies[index];

        const auto range =
            collisionGrid.CellsOverlapping(
                {first.position.x,first.position.y},
                searchRadius);

        for (int row = range.minimum.row;
             row <= range.maximum.row;
             ++row) {
            for (int column = range.minimum.column;
                 column <= range.maximum.column;
                 ++column) {
                for (AntPhysicsBody *second : collisionGrid.GetIds({column,row})) {
                    if (second->antId==first.antId || second->colonyId!=first.colonyId)continue;

                    const pipeframe::Vector2f toCandidate =
                        second->position -
                        first.position;

                    const float distanceSquared =
                        toCandidate.x *
                            toCandidate.x +
                        toCandidate.y *
                            toCandidate.y;

                    if (distanceSquared >
                        searchRadius *
                            searchRadius) {
                        continue;
                    }

                    const float forwardDot =
                        first.direction.x *
                            toCandidate.x +
                        first.direction.y *
                            toCandidate.y;

                    if (forwardDot <= 0.0f) {
                        continue;
                    }

                    const std::optional<float>
                        collisionTime =
                            CalculateTimeToCollision(
                                first,
                                *second);

                    if (!collisionTime ||
                        *collisionTime >=
                            MaximumTimeToCollision) {
                        continue;
                    }

                    futureCollisions.push_back({
                        first.id,
                        second->id,
                        *collisionTime,
                    });
                }
            }
        }
    }
}

void AntAvoidanceSystem::
SolveFutureCollisions(
    const float deltaTime
) {
    constexpr float MinimumDistanceSquared{
        0.0001f
    };

    for (const FutureCollision &collision :
         futureCollisions) {
        AntPhysicsBody *first =
            physicsWorld.FindBody(
                collision.firstBodyId);

        const AntPhysicsBody *second =
            physicsWorld.FindBody(
                collision.secondBodyId);

        if (first == nullptr ||
            second == nullptr) {
            continue;
        }

        const float previousTime =
            std::floor(
                collision.timeToCollision /
                deltaTime) *
            deltaTime;

        const float nextTime =
            previousTime +
            deltaTime;

        const pipeframe::Vector2f firstBefore =
            first->position +
            first->velocity *
                previousTime;

        const pipeframe::Vector2f secondBefore =
            second->position +
            second->velocity *
                previousTime;

        const pipeframe::Vector2f firstAfter =
            first->position +
            first->velocity *
                nextTime;

        const pipeframe::Vector2f secondAfter =
            second->position +
            second->velocity *
                nextTime;

        const pipeframe::Vector2f collisionVector =
            firstAfter -
            secondAfter;

        const float distanceSquared =
            collisionVector.x *
                collisionVector.x +
            collisionVector.y *
                collisionVector.y;

        if (distanceSquared >= 1.0f ||
            distanceSquared <=
                MinimumDistanceSquared) {
            continue;
        }

        const float firstWeight =
            1.0f /
            std::max(
                0.0001f,
                first->mass);

        const float secondWeight =
            1.0f /
            std::max(
                0.0001f,
                second->mass);

        const float inverseWeightSum =
            1.0f /
            (firstWeight +
             secondWeight);

        const float distance =
            std::sqrt(
                distanceSquared);

        const float overlap =
            1.0f -
            distance;

        const pipeframe::Vector2f normal =
            collisionVector /
            distance;

        const pipeframe::Vector2f solvedFirst =
            firstAfter +
            normal *
                (overlap *
                 firstWeight *
                 inverseWeightSum);

        const pipeframe::Vector2f solvedSecond =
            secondAfter -
            normal *
                (overlap *
                 secondWeight *
                 inverseWeightSum);

        const pipeframe::Vector2f displacement =
            (solvedFirst - firstBefore) -
            (solvedSecond - secondBefore);

        const float normalProjection =
            displacement.x *
                normal.x +
            displacement.y *
                normal.y;

        const pipeframe::Vector2f tangential =
            displacement -
            normal *
                normalProjection;

        const float farRatio =
            previousTime /
            MaximumTimeToCollision;

        const float stiffness =
            0.25f *
            static_cast<float>(
                SliceCount) *
            std::exp(-farRatio);

        const pipeframe::Vector2f correction =
            tangential *
            stiffness *
            (firstWeight *
             inverseWeightSum);

        first->position += correction;
    }
}

} // namespace ant_simulation
