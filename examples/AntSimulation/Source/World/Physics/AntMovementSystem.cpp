#include "World/Physics/AntMovementSystem.h"
#include "Components/ColonySettingsComponent.h"
#include "Components/ColonyStateComponent.h"
#include <PipeFrame/Simulation/Steering2D.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <vector>

namespace ant_simulation {
AntMovementSystem::AntMovementSystem(AntQuery &s, AntBodySystem &p, ContactSolver &c, const AntConfiguration &config)
    : store(s), physics(p), contacts(c), configuration(config), avoidance(s, p, config) {
    if (configuration.antUpdateWorkerCount > 1)
        workers = std::make_unique<pipeframe::ThreadPool>(configuration.antUpdateWorkerCount);
}
AntMovementResult AntMovementSystem::Update(const float delta) {
    AntMovementResult result;
    if (delta <= 0)
        return result;
    using Clock = std::chrono::steady_clock;
    using Ms = std::chrono::duration<float, std::milli>;
    const auto start = Clock::now();
    physics.Synchronize();
    AdvanceState(delta);
    colonySpeeds.clear();
    store.GetWorld().Each<ColonyStateComponent, ColonySettingsComponent>(
        [&](auto, const auto &colony, const auto &settings) {
            colonySpeeds.emplace(colony.id, static_cast<float>(settings.speed));
        });
    const auto energyAccess = store.GetWorld().BorrowComponents<pipeframe::EnergyComponent>();
    const auto foragingAccess = store.GetWorld().BorrowComponents<ForagingComponent>();
    const auto encounters = store.GetWorld().BorrowComponents<AntEncounterComponent>();
    for (AntView &ant : store.GetAnts()) {
        auto *energy = energyAccess.Get(ant.GetId());
        if (!energy || energy->IsDepleted()) {
            physics.SetVelocity(ant.GetId(), {});
            continue;
        }
        if (auto *encounter = encounters.Get(ant.GetId()); encounter && !encounter->opponentId)
            WriteMovementCommand(ant);
    }
    const auto avoidanceStart = Clock::now();
    result.preparationTimeMs = Ms(avoidanceStart - start).count();
    result.futureCollisions = avoidance.Update(delta);
    const auto physicsStart = Clock::now();
    result.avoidanceTimeMs = Ms(physicsStart - avoidanceStart).count();
    physics.Step(delta);
    const auto solved = contacts.Solve(physics);
    result.antContacts = solved.antContacts;
    result.wallConstraints = solved.wallConstraints;
    for (AntView &ant : store.GetAnts())
        if (auto *energy = energyAccess.Get(ant.GetId()); energy && !energy->IsDepleted())
            if (auto *foraging = foragingAccess.Get(ant.GetId()))
                ReadMovementResult(ant, *foraging, delta);
    result.physicsTimeMs = Ms(Clock::now() - physicsStart).count();
    return result;
}
void AntMovementSystem::AdvanceState(const float delta) {
    auto ants = store.GetAnts();
    const auto energyAccess = store.GetWorld().BorrowComponents<pipeframe::EnergyComponent>();
    const auto run = [ants, delta, energyAccess](std::size_t begin, std::size_t end) {
        for (auto i = begin; i < end; ++i) {
            auto *energy = energyAccess.Get(ants[i].GetId());
            if (energy && !energy->IsDepleted()) {
                ants[i].Update(delta);
                energy->Consume(delta);
            }
        }
    };
    if (!workers || ants.size() < 2) {
        run(0, ants.size());
        return;
    }
    const auto count = std::min<std::size_t>(configuration.antUpdateWorkerCount, ants.size());
    const auto chunk = (ants.size() + count - 1) / count;
    std::vector<std::future<void>> futures;
    for (std::size_t begin = 0; begin < ants.size(); begin += chunk)
        futures.emplace_back(workers->Submit([=] { run(begin, std::min(ants.size(), begin + chunk)); }));
    for (auto &future : futures)
        future.get();
}
void AntMovementSystem::WriteMovementCommand(AntView &ant) {
    auto *body = physics.FindAntBody(ant.GetId());
    if (!body)
        return;
    physics.SetVelocity(ant.GetId(), pipeframe::SeekVelocity(body->position, body->velocity, ant.GetTarget(),
                                                             colonySpeeds.contains(ant.GetColonyId())
                                                                 ? colonySpeeds.at(ant.GetColonyId())
                                                                 : configuration.antSpeed,
                                                             0.3f));
}
void AntMovementSystem::ReadMovementResult(AntView &ant, ForagingComponent &foraging, const float delta) const {
    const auto *body = physics.FindAntBody(ant.GetId());
    if (!body)
        return;
    ant.Motion().velocity = body->velocity;
    const float distance = std::sqrt(body->lastMove.x * body->lastMove.x + body->lastMove.y * body->lastMove.y);
    foraging.distanceToTarget = std::max(0.0f, foraging.distanceToTarget - distance);
    ant.Motion().travelDistance += distance;
    ant.Motion().speed = delta > 0 ? distance / delta : 0;
    if (ant.Motion().speed > 0)
        ant.SetDirection(body->velocity / ant.Motion().speed);
    ant.UpdateLegs(delta);
}
pipeframe::Vector2f AntMovementSystem::Normalize(const pipeframe::Vector2f value) {
    const float square = value.x * value.x + value.y * value.y;
    return square <= 0 ? pipeframe::Vector2f{} : value / std::sqrt(square);
}
} // namespace ant_simulation
