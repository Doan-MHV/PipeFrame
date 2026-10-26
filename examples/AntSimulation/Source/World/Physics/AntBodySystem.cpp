#include "World/Physics/AntBodySystem.h"

#include <PipeFrame/Physics/Physics2D.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace ant_simulation {

AntBodySystem::AntBodySystem(AntQuery &newAntQuery, const AntConfiguration &newConfiguration)
    : antStore(newAntQuery), configuration(newConfiguration) {}

void AntBodySystem::Synchronize() {
    for (std::size_t index = storage.Bodies().size(); index > 0; --index) {
        const std::size_t bodyIndex = index - 1;

        AntView *ant = antStore.Find(storage.Bodies()[bodyIndex].antId);

        if (ant != nullptr && !ant->IsDead()) {
            continue;
        }

        if (ant != nullptr) {
            ant->Identity().physicsObjectId = InvalidPhysicsBodyId;
        }

        RemoveBodyAt(bodyIndex);
    }

    for (AntView &ant : antStore.GetAnts()) {
        if (ant.IsDead()) {
            continue;
        }

        AntPhysicsBody *body = FindAntBody(ant.GetId());

        if (body == nullptr) {
            body = &CreateBody(ant);
        }

        body->colonyId = ant.GetColonyId();

        body->mass = ant.GetMass();

        body->direction = {ant.GetDirection().x, ant.GetDirection().y};

        ant.Identity().physicsObjectId = body->id;
    }
}

void AntBodySystem::Step(const float deltaTime) {
    if (deltaTime <= 0.0f) {
        return;
    }

    const float maximumMove = configuration.antSpeed * deltaTime;

    for (AntPhysicsBody &body : storage.Bodies()) {
        pipeframe::IntegrateBody(body, deltaTime, maximumMove);
        const float collisionRadius = body.radius;
        body.radius = 2.5f;
        pipeframe::ConstrainToBounds(
            body, {{0.0f, 0.0f},
                   {static_cast<float>(configuration.worldSize.x), static_cast<float>(configuration.worldSize.y)}});
        body.radius = collisionRadius;
        if (pipeframe::LengthSquared(body.velocity) > 0.0f)
            body.direction = pipeframe::NormalizeOr(body.velocity);
    }
}

void AntBodySystem::SynchronizeAntPositions() {
    for (const AntPhysicsBody &body : storage.Bodies()) {
        AntView *ant = antStore.Find(body.antId);

        if (ant == nullptr || ant->IsDead()) {
            continue;
        }

        ant->SetPosition({body.position.x, body.position.y});
    }
}

AntPhysicsBody *AntBodySystem::FindBody(const PhysicsBodyId id) { return storage.Find(id); }

const AntPhysicsBody *AntBodySystem::FindBody(const PhysicsBodyId id) const { return storage.Find(id); }

AntPhysicsBody *AntBodySystem::FindAntBody(const AntId antId) {
    const auto iterator = antBodyIds.find(antId);

    if (iterator == antBodyIds.end()) {
        return nullptr;
    }

    return FindBody(iterator->second);
}

const AntPhysicsBody *AntBodySystem::FindAntBody(const AntId antId) const {
    const auto iterator = antBodyIds.find(antId);

    if (iterator == antBodyIds.end()) {
        return nullptr;
    }

    return FindBody(iterator->second);
}

bool AntBodySystem::SetVelocity(const AntId antId, const pipeframe::Vector2f velocity) {
    AntPhysicsBody *body = FindAntBody(antId);

    if (body == nullptr) {
        return false;
    }

    body->velocity = velocity;
    return true;
}

bool AntBodySystem::SetDirection(const AntId antId, const pipeframe::Vector2f direction) {
    AntPhysicsBody *body = FindAntBody(antId);

    if (body == nullptr) {
        return false;
    }

    body->direction = pipeframe::NormalizeOr(direction);

    return true;
}

bool AntBodySystem::Teleport(const AntId antId, const pipeframe::Vector2f position) {
    AntPhysicsBody *body = FindAntBody(antId);

    if (body == nullptr) {
        return false;
    }

    body->position = position;
    body->previousPosition = position;
    body->lastMove = {};

    return true;
}

std::size_t AntBodySystem::GetBodyCount() const { return storage.Bodies().size(); }

std::span<AntPhysicsBody> AntBodySystem::GetBodies() { return storage.Bodies(); }

std::span<const AntPhysicsBody> AntBodySystem::GetBodies() const { return storage.Bodies(); }

void AntBodySystem::Clear() {
    for (AntPhysicsBody &body : storage.Bodies()) {
        AntView *ant = antStore.Find(body.antId);

        if (ant != nullptr) {
            ant->Identity().physicsObjectId = InvalidPhysicsBodyId;
        }
    }

    storage.Clear(true);
    antBodyIds.clear();
}

AntPhysicsBody &AntBodySystem::CreateBody(AntView &ant) {
    AntPhysicsBody body;

    body.antId = ant.GetId();
    body.colonyId = ant.GetColonyId();

    body.position = {ant.GetPosition().x, ant.GetPosition().y};

    body.previousPosition = body.position;

    body.direction = {ant.GetDirection().x, ant.GetDirection().y};

    body.mass = ant.GetMass();

    const auto id = storage.Create(body);

    antBodyIds.emplace(ant.GetId(), id);

    ant.Identity().physicsObjectId = id;

    return *storage.Find(id);
}

void AntBodySystem::RemoveBodyAt(const std::size_t index) {
    if (index >= storage.Bodies().size()) {
        return;
    }

    const PhysicsBodyId removedBodyId = storage.Bodies()[index].id;

    const AntId removedAntId = storage.Bodies()[index].antId;

    storage.Remove(removedBodyId);

    antBodyIds.erase(removedAntId);
}

} // namespace ant_simulation
