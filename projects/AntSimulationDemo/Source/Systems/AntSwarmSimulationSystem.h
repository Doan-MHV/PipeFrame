#ifndef PIPEFRAME_ANT_SWARM_SIMULATION_SYSTEM_H
#define PIPEFRAME_ANT_SWARM_SIMULATION_SYSTEM_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <string>

#include <glm/geometric.hpp>

#include "Components/AntColonyComponent.h"
#include "Components/AntSwarmComponent.h"
#include "Components/AttributesComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Components/FoodSourceComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Map/TileMap.h"
#include "Navigation/GridRaycaster.h"
#include "Project/ProjectConfig.h"
#include "Physics/DenseCircleSolver.h"
#include "Simulation/ProjectModule.h"
#include "Fields/FieldGridSolver.h"
#include "Simulations/AntSwarmSimulation.h"

class AntSwarmSimulationSystem : public SimulationSystem<AntSwarmSimulation>
{
public:
    void OnWorldLoaded(AntSwarmSimulation& simulation, Registry& registry) override
    {
        (void)simulation;
        EnsureProjectComponents(registry);
    }

    void Update(AntSwarmSimulation& simulation, ProjectRuntimeContext& context) override
    {
        EnsureProjectComponents(context.registry);
        ApplySettingsFromSwarm(simulation, context.registry);
        HideDepletedFoodSources(context.registry);
        EnsureFields(simulation, context.tileMap, context.projectConfig);

        SimulationConfig foodConfig = context.projectConfig.simulation;
        foodConfig.fieldDecayPerSecond = simulation.Settings().foodFieldDecayPerSecond;
        SimulationConfig homeConfig = context.projectConfig.simulation;
        homeConfig.fieldDecayPerSecond = simulation.Settings().homeFieldDecayPerSecond;

        FieldGridSolver::Decay(simulation.Fields()["food"], context.deltaTime, foodConfig);
        FieldGridSolver::Decay(simulation.Fields()["home"], context.deltaTime, homeConfig);

        RefreshPersistentHomeMarkers(simulation, context.registry, context.tileMap, context.projectConfig);
        SpawnFromColonies(simulation, context.registry, context.tileMap, context.deltaTime);

        for (AntAgent& ant : simulation.Agents())
        {
            StepAnt(
                simulation,
                ant,
                context.registry,
                context.tileMap,
                context.projectConfig,
                static_cast<float>(context.deltaTime)
            );
        }
        RemoveDeadAgents(simulation);

        DenseCircleSolverConfig physicsConfig;
        physicsConfig.radius = simulation.Settings().agentRadius;
        physicsConfig.response = simulation.Settings().collisionPushStrength;
        physicsConfig.cellSize = simulation.Settings().agentRadius * 2.5f;
        physicsConfig.iterations = simulation.Settings().collisionIterations;
        DenseCircleSolver::Resolve(
            simulation.Agents(),
            physicsConfig,
            [&](glm::vec2 from, glm::vec2 to)
            {
                return CanTravel(simulation, context.tileMap, from, to);
            }
        );
    }

private:
    struct SampleResult
    {
        bool valid = false;
        glm::vec2 position = glm::vec2(0.0f);
        glm::vec2 direction = glm::vec2(1.0f, 0.0f);
        float distance = 0.0f;
        float intensity = 0.0f;
        bool foundFood = false;
    };

    void EnsureProjectComponents(Registry& registry)
    {
        for (Entity entity : registry.GetAllEntities())
        {
            if (entity.BelongsToGroup("colonies") && !entity.HasComponent<AntColonyComponent>())
            {
                entity.AddComponent<AntColonyComponent>();
            }

            if (entity.BelongsToGroup("food") && !entity.HasComponent<FoodSourceComponent>())
            {
                entity.AddComponent<FoodSourceComponent>();
            }

            if (entity.BelongsToGroup("swarms") && !entity.HasComponent<AntSwarmComponent>())
            {
                entity.AddComponent<AntSwarmComponent>();
            }

            if ((entity.BelongsToGroup("colonies") || entity.BelongsToGroup("food")) &&
                !entity.HasComponent<AttributesComponent>())
            {
                entity.AddComponent<AttributesComponent>();
            }
        }
    }

    void ApplySettingsFromSwarm(AntSwarmSimulation& simulation, Registry& registry)
    {
        for (Entity entity : registry.GetAllEntities())
        {
            if (!entity.HasComponent<AntSwarmComponent>())
            {
                continue;
            }

            const auto& swarm = entity.GetComponent<AntSwarmComponent>();
            auto& settings = simulation.Settings();
            settings.speed = swarm.speed;
            settings.maxEnergy = swarm.maxEnergy;
            settings.refillEnergyRatio = swarm.refillEnergyRatio;
            settings.pickupRadius = swarm.pickupRadius;
            settings.dropoffRadius = swarm.dropoffRadius;
            settings.sensorDistanceMin = swarm.sensorDistanceMin;
            settings.sensorDistanceMax = swarm.sensorDistanceMax;
            settings.followerFovRadians = swarm.followerFovRadians;
            settings.explorerFovRadians = swarm.explorerFovRadians;
            settings.explorerRatio = swarm.explorerRatio;
            settings.followerSampleCount = swarm.followerSampleCount;
            settings.explorerSampleCount = swarm.explorerSampleCount;
            settings.markerDistance = swarm.markerDistance;
            settings.trailDepositAmount = swarm.trailDepositAmount;
            settings.trailRadius = swarm.trailRadius;
            settings.blockedDegradeAmount = swarm.blockedDegradeAmount;
            settings.foodFieldDecayPerSecond = swarm.foodTrailDecayPerSecond;
            settings.homeFieldDecayPerSecond = swarm.homeTrailDecayPerSecond;
            settings.maxTurnRadiansPerSecond = swarm.maxTurnRadiansPerSecond;
            settings.agentRadius = swarm.agentRadius;
            settings.collisionPushStrength = swarm.collisionPushStrength;
            settings.collisionIterations = swarm.collisionIterations;
            simulation.Reserve(static_cast<std::size_t>(std::max(swarm.maxAgents, 0)));
            return;
        }
    }

    void HideDepletedFoodSources(Registry& registry)
    {
        for (Entity entity : registry.GetAllEntities())
        {
            if (!entity.HasComponent<FoodSourceComponent>() || GetFoodAmount(entity) > 0)
            {
                continue;
            }

            if (entity.HasComponent<SpriteComponent>())
            {
                entity.RemoveComponent<SpriteComponent>();
            }

            if (entity.HasComponent<BoxColliderComponent>())
            {
                entity.RemoveComponent<BoxColliderComponent>();
            }
        }
    }

    void EnsureFields(
        AntSwarmSimulation& simulation,
        const TileMap& tileMap,
        const ProjectConfig& projectConfig
    )
    {
        FieldGridSolver::EnsureMatchesTileMap(simulation.Fields()["food"], tileMap, projectConfig.simulation);
        FieldGridSolver::EnsureMatchesTileMap(simulation.Fields()["home"], tileMap, projectConfig.simulation);
    }

    void RefreshPersistentHomeMarkers(
        AntSwarmSimulation& simulation,
        Registry& registry,
        const TileMap& tileMap,
        const ProjectConfig& projectConfig
    )
    {
        for (Entity colony : registry.GetAllEntities())
        {
            if (!colony.HasComponent<AntColonyComponent>() || !colony.HasComponent<TransformComponent>())
            {
                continue;
            }

            const glm::vec2 position = colony.GetComponent<TransformComponent>().position;
            FieldDepositCommand command;
            command.fieldName = "home";
            command.x = position.x;
            command.y = position.y;
            command.amount = simulation.Settings().markerMaxIntensity;
            command.radius = simulation.Settings().dropoffRadius;
            command.falloff = "linear";
            FieldGridSolver::ApplyDeposit(
                simulation.Fields()["home"],
                tileMap,
                projectConfig.simulation,
                command
            );
        }
    }

    void SpawnFromColonies(
        AntSwarmSimulation& simulation,
        Registry& registry,
        const TileMap& tileMap,
        double deltaTime
    )
    {
        simulation.SpawnTimer() += deltaTime;

        for (Entity colony : registry.GetAllEntities())
        {
            if (!colony.HasComponent<AntColonyComponent>() || !colony.HasComponent<TransformComponent>())
            {
                continue;
            }

            auto& colonyComponent = colony.GetComponent<AntColonyComponent>();
            const double spawnInterval = std::max(colonyComponent.spawnInterval, 0.001);
            const int maxAgents = GetConfiguredMaxAgents(simulation, registry, colonyComponent.maxAnts);

            while (simulation.Count() < static_cast<std::size_t>(std::max(maxAgents, 0)) &&
                simulation.SpawnTimer() >= spawnInterval)
            {
                simulation.SpawnTimer() -= spawnInterval;
                const glm::vec2 colonyPosition = colony.GetComponent<TransformComponent>().position;
                glm::vec2 spawnDirection = RandomOpenDirection(simulation, tileMap, colonyPosition);

                AntAgent ant;
                ant.id = simulation.AllocateAgentId();
                ant.position = colonyPosition + spawnDirection * simulation.Settings().spawnRadius;
                if (!CanOccupy(simulation, tileMap, ant.position))
                {
                    ant.position = colonyPosition + colonyComponent.spawnOffset;
                }
                ant.direction = spawnDirection;
                ant.steeringDirection = spawnDirection;
                ant.velocity = spawnDirection * simulation.Settings().speed;
                ant.homePosition = colonyPosition;
                ant.target = ant.position;
                ant.lastMarkerPosition = colonyPosition;
                ant.energy = simulation.Settings().maxEnergy;
                ant.role = Random01(simulation) < simulation.Settings().explorerRatio
                    ? AntRole::Explorer
                    : AntRole::Follower;
                simulation.Agents().push_back(ant);
                colonyComponent.spawnedAnts++;
            }

            return;
        }
    }

    void RemoveDeadAgents(AntSwarmSimulation& simulation)
    {
        auto& agents = simulation.Agents();
        agents.erase(
            std::remove_if(
                agents.begin(),
                agents.end(),
                [](const AntAgent& ant)
                {
                    return ant.energy <= 0.0f;
                }
            ),
            agents.end()
        );
    }

    int GetConfiguredMaxAgents(
        const AntSwarmSimulation& simulation,
        Registry& registry,
        int fallbackMaxAgents
    ) const
    {
        (void)simulation;

        for (Entity entity : registry.GetAllEntities())
        {
            if (entity.HasComponent<AntSwarmComponent>())
            {
                return entity.GetComponent<AntSwarmComponent>().maxAgents;
            }
        }

        return fallbackMaxAgents;
    }

    void StepAnt(
        AntSwarmSimulation& simulation,
        AntAgent& ant,
        Registry& registry,
        const TileMap& tileMap,
        const ProjectConfig& projectConfig,
        float deltaTime
    )
    {
        ant.age += deltaTime;
        ant.walkTime += deltaTime;
        ant.timeSinceMarker += deltaTime;
        ant.energy -= deltaTime;
        ant.animationPhase += deltaTime * simulation.Settings().speed * 0.05f;

        RepairInvalidPosition(simulation, ant, tileMap);
        TryPickupOrDropoff(simulation, ant, registry);

        const bool blocked = IsBlocked(simulation, ant);
        ant.blocked = blocked;
        if (TargetReached(ant) || blocked)
        {
            if (blocked)
            {
                DegradeFocusedMarker(simulation, ant, tileMap, projectConfig);
                EscapeBlockedAnt(simulation, ant, registry, tileMap);
            }
            else
            {
                SampleWorld(simulation, ant, registry, tileMap);
            }
        }

        const glm::vec2 previousPosition = ant.position;
        const glm::vec2 targetDirection = SafeNormalize(ant.target - ant.position, ant.direction);
        const glm::vec2 moveDirection = TurnTowards(
            ant.direction,
            targetDirection,
            simulation.Settings().maxTurnRadiansPerSecond * deltaTime
        );

        const glm::vec2 desiredVelocity = moveDirection * simulation.Settings().speed;
        constexpr float blendRatio = 0.1f;
        ant.velocity = ant.velocity * (1.0f - blendRatio) + desiredVelocity * blendRatio;

        const float moveDistance = glm::length(ant.velocity) * deltaTime;
        const glm::vec2 nextPosition = ant.position + ant.velocity * deltaTime;
        if (CanTravel(simulation, tileMap, ant.position, nextPosition))
        {
            ant.position = nextPosition;
            ant.direction = SafeNormalize(ant.velocity, moveDirection);
            ant.distanceToTarget -= moveDistance;
        }
        else
        {
            DegradeFocusedMarker(simulation, ant, tileMap, projectConfig);
            EscapeBlockedAnt(simulation, ant, registry, tileMap);
        }

        const float movedDistance = glm::length(ant.position - previousPosition);
        ant.currentSpeed = movedDistance / std::max(deltaTime, 0.0001f);
        ant.totalTravelDistance += movedDistance;
        if (movedDistance < 0.2f)
        {
            ant.stuckTime += deltaTime;
        }
        else
        {
            ant.stuckTime = 0.0;
        }

        UpdateMarker(simulation, ant, tileMap, projectConfig);
    }

    bool TargetReached(const AntAgent& ant) const
    {
        return ant.distanceToTarget <= 0.0f || glm::length(ant.target - ant.position) < 4.0f;
    }

    bool IsBlocked(const AntSwarmSimulation& simulation, const AntAgent& ant) const
    {
        const double markerTime = simulation.Settings().markerDistance /
            std::max(simulation.Settings().speed, 0.001f);
        return ant.timeSinceMarker > markerTime * 10.0 || ant.stuckTime > simulation.Settings().stuckTimeout;
    }

    void SampleWorld(
        AntSwarmSimulation& simulation,
        AntAgent& ant,
        Registry& registry,
        const TileMap& tileMap
    )
    {
        const float fovScale = ant.blocked ? 2.0f : 1.0f;
        const float objectiveFov = GetObjectiveFov(simulation, ant) * fovScale;
        const int sampleCount = GetSampleCount(simulation, ant);

        SampleResult objective = GetBestObjectiveSample(simulation, ant, registry, tileMap, objectiveFov, sampleCount);
        if (objective.valid)
        {
            SetTarget(ant, objective);
            return;
        }

        if (ant.state == AntState::ToFood &&
            ant.energy < simulation.Settings().maxEnergy * simulation.Settings().refillEnergyRatio)
        {
            ant.state = AntState::ToHomeNoFood;
            ant.walkTime = 0.0f;
            ant.direction = -ant.direction;
            ant.velocity = ant.direction * simulation.Settings().speed;
            ant.target = ant.position + ant.direction * simulation.Settings().sensorDistanceMin;
            ant.distanceToTarget = simulation.Settings().sensorDistanceMin;
            return;
        }

        if (TrySetDirectHomeTarget(simulation, ant, registry, tileMap))
        {
            return;
        }

        SampleResult fallback = GetBestFallbackSample(simulation, ant, tileMap, objectiveFov, sampleCount);
        if (fallback.valid)
        {
            SetTarget(ant, fallback);
            return;
        }

        const glm::vec2 direction = RandomOpenDirection(simulation, tileMap, ant.position);
        SetTarget(
            ant,
            {
                .valid = true,
                .position = ant.position + direction * simulation.Settings().sensorDistanceMin,
                .direction = direction,
                .distance = simulation.Settings().sensorDistanceMin
            }
        );
    }

    SampleResult GetBestObjectiveSample(
        AntSwarmSimulation& simulation,
        const AntAgent& ant,
        Registry& registry,
        const TileMap& tileMap,
        float fov,
        int sampleCount
    )
    {
        SampleResult best;
        const std::string fieldName = GetMarkerFocus(ant);

        for (int index = 0; index < sampleCount; index++)
        {
            SampleResult sample = GetSample(simulation, ant, tileMap, fov);
            if (!sample.valid)
            {
                continue;
            }

            if (fieldName == "food")
            {
                Entity food = FindNearestFood(registry, sample.position);
                if (food.GetId() >= 0 &&
                    glm::length(GetEntityPosition(food) - sample.position) <= simulation.Settings().pickupRadius * 2.0f)
                {
                    sample.foundFood = true;
                    sample.intensity = simulation.Settings().markerMaxIntensity;
                    return sample;
                }
            }

            sample.intensity = SampleField(simulation, fieldName, sample.position);
            sample.intensity *= MarkerSamplingCoefficient(simulation, tileMap, sample.position);
            if (sample.intensity > best.intensity)
            {
                best = sample;
            }
        }

        return best.intensity > 0.0001f ? best : SampleResult{};
    }

    SampleResult GetBestFallbackSample(
        AntSwarmSimulation& simulation,
        const AntAgent& ant,
        const TileMap& tileMap,
        float fov,
        int sampleCount
    )
    {
        SampleResult best;
        for (int index = 0; index < sampleCount; index++)
        {
            SampleResult sample = GetSample(simulation, ant, tileMap, fov);
            if (!sample.valid)
            {
                continue;
            }

            if (sample.distance > best.distance)
            {
                best = sample;
            }
        }

        return best;
    }

    SampleResult GetSample(
        AntSwarmSimulation& simulation,
        const AntAgent& ant,
        const TileMap& tileMap,
        float fov
    )
    {
        const float baseAngle = std::atan2(ant.direction.y, ant.direction.x);
        const float angle = baseAngle + RandomRange(simulation, -fov * 0.5f, fov * 0.5f);
        const glm::vec2 direction(std::cos(angle), std::sin(angle));
        const float minDistance = simulation.Settings().sensorDistanceMin;
        const float maxDistance = std::max(simulation.Settings().sensorDistanceMax, minDistance);
        const float distanceSquared = RandomRange(simulation, minDistance * minDistance, maxDistance * maxDistance);
        const float distance = std::sqrt(distanceSquared);
        const glm::vec2 position = ant.position + direction * distance;

        if (!CanTravel(simulation, tileMap, ant.position, position))
        {
            return {};
        }

        return {
            .valid = true,
            .position = position,
            .direction = direction,
            .distance = distance
        };
    }

    void SetTarget(AntAgent& ant, const SampleResult& sample)
    {
        ant.target = sample.position;
        ant.distanceToTarget = sample.distance * 0.5f;
        ant.steeringDirection = sample.direction;
    }

    void EscapeBlockedAnt(
        AntSwarmSimulation& simulation,
        AntAgent& ant,
        Registry& registry,
        const TileMap& tileMap
    )
    {
        if (TrySetDirectHomeTarget(simulation, ant, registry, tileMap))
        {
            return;
        }

        const glm::vec2 direction = RandomOpenDirection(simulation, tileMap, ant.position);
        const float distance = simulation.Settings().sensorDistanceMin;
        ant.direction = direction;
        ant.steeringDirection = direction;
        ant.velocity = direction * simulation.Settings().speed;
        ant.target = ant.position + direction * distance;
        ant.distanceToTarget = distance;
        ant.stuckTime = 0.0;
    }

    bool TrySetDirectHomeTarget(
        AntSwarmSimulation& simulation,
        AntAgent& ant,
        Registry& registry,
        const TileMap& tileMap
    )
    {
        if (ant.state == AntState::ToFood)
        {
            return false;
        }

        Entity colony = FindNearestColony(registry, ant.position);
        if (colony.GetId() < 0)
        {
            return false;
        }

        const glm::vec2 colonyPosition = GetEntityPosition(colony);
        const glm::vec2 toHome = colonyPosition - ant.position;
        const float distance = glm::length(toHome);
        if (distance <= simulation.Settings().dropoffRadius)
        {
            return false;
        }

        const glm::vec2 direction = SafeNormalize(toHome, ant.direction);
        const float targetDistance = std::min(distance, simulation.Settings().sensorDistanceMax);
        const glm::vec2 target = ant.position + direction * targetDistance;
        if (!CanTravel(simulation, tileMap, ant.position, target))
        {
            return false;
        }

        ant.direction = direction;
        ant.steeringDirection = direction;
        ant.velocity = direction * simulation.Settings().speed;
        ant.target = target;
        ant.distanceToTarget = targetDistance;
        ant.stuckTime = 0.0;
        return true;
    }

    void TryPickupOrDropoff(AntSwarmSimulation& simulation, AntAgent& ant, Registry& registry)
    {
        if (ant.state == AntState::ToFood)
        {
            Entity food = FindNearestFood(registry, ant.position);
            if (food.GetId() >= 0 &&
                glm::length(GetEntityPosition(food) - ant.position) <= simulation.Settings().pickupRadius &&
                TakeFood(food))
            {
                ant.state = AntState::ToHomeWithFood;
                ant.walkTime = 0.0f;
                ant.energy = simulation.Settings().maxEnergy;
                ant.direction = -ant.direction;
                ant.velocity = ant.direction * simulation.Settings().speed;
                ant.target = ant.position + ant.direction * simulation.Settings().sensorDistanceMin;
                ant.distanceToTarget = simulation.Settings().sensorDistanceMin;
            }
            return;
        }

        Entity colony = FindNearestColony(registry, ant.position);
        if (colony.GetId() >= 0 &&
            glm::length(GetEntityPosition(colony) - ant.position) <= simulation.Settings().dropoffRadius)
        {
            if (ant.state == AntState::ToHomeWithFood)
            {
                if (!colony.HasComponent<AttributesComponent>())
                {
                    colony.AddComponent<AttributesComponent>();
                }

                auto& attributes = colony.GetComponent<AttributesComponent>().values;
                attributes["stored_food"] = attributes.value("stored_food", 0) + 1;
                ant.collectedFood++;
            }

            ant.state = AntState::ToFood;
            ant.walkTime = 0.0f;
            ant.energy = simulation.Settings().maxEnergy;
            ant.direction = -ant.direction;
            ant.velocity = ant.direction * simulation.Settings().speed;
            ant.target = ant.position + ant.direction * simulation.Settings().sensorDistanceMin;
            ant.distanceToTarget = simulation.Settings().sensorDistanceMin;
        }
    }

    void UpdateMarker(
        AntSwarmSimulation& simulation,
        AntAgent& ant,
        const TileMap& tileMap,
        const ProjectConfig& projectConfig
    )
    {
        if (glm::length(ant.position - ant.lastMarkerPosition) < simulation.Settings().markerDistance)
        {
            return;
        }

        ant.lastMarkerPosition = ant.position;
        ant.timeSinceMarker = 0.0f;

        const std::string marker = GetDropMarker(ant);
        if (marker.empty())
        {
            return;
        }

        const double intensity = simulation.Settings().trailDepositAmount *
            std::pow(1.0 - std::min(GetFieldDecay(simulation, marker), 0.95), simulation.Settings().markerDecayPower * ant.walkTime);
        DepositField(simulation, tileMap, projectConfig, marker, ant.position, intensity, simulation.Settings().trailRadius);
    }

    void DegradeFocusedMarker(
        AntSwarmSimulation& simulation,
        const AntAgent& ant,
        const TileMap& tileMap,
        const ProjectConfig& projectConfig
    )
    {
        DepositField(
            simulation,
            tileMap,
            projectConfig,
            GetMarkerFocus(ant),
            ant.position,
            -simulation.Settings().blockedDegradeAmount,
            simulation.Settings().trailRadius * 2.0f
        );
    }

    void DepositField(
        AntSwarmSimulation& simulation,
        const TileMap& tileMap,
        const ProjectConfig& projectConfig,
        const std::string& fieldName,
        glm::vec2 position,
        double amount,
        float radius
    )
    {
        FieldDepositCommand command;
        command.fieldName = fieldName;
        command.x = position.x;
        command.y = position.y;
        command.amount = amount;
        command.radius = radius;
        command.falloff = "linear";
        FieldGridSolver::ApplyDeposit(
            simulation.Fields()[fieldName],
            tileMap,
            projectConfig.simulation,
            command
        );
    }

    std::string GetDropMarker(const AntAgent& ant) const
    {
        if (ant.state == AntState::ToFood)
        {
            return "home";
        }

        if (ant.state == AntState::ToHomeWithFood)
        {
            return "food";
        }

        return "";
    }

    std::string GetMarkerFocus(const AntAgent& ant) const
    {
        return ant.state == AntState::ToFood ? "food" : "home";
    }

    float GetObjectiveFov(const AntSwarmSimulation& simulation, const AntAgent& ant) const
    {
        if (ant.state != AntState::ToFood)
        {
            return simulation.Settings().followerFovRadians;
        }

        return ant.role == AntRole::Follower
            ? simulation.Settings().followerFovRadians
            : simulation.Settings().explorerFovRadians;
    }

    int GetSampleCount(const AntSwarmSimulation& simulation, const AntAgent& ant) const
    {
        if (ant.state != AntState::ToFood)
        {
            return simulation.Settings().followerSampleCount;
        }

        return ant.role == AntRole::Follower
            ? simulation.Settings().followerSampleCount
            : simulation.Settings().explorerSampleCount;
    }

    double GetFieldDecay(const AntSwarmSimulation& simulation, const std::string& fieldName) const
    {
        return fieldName == "food"
            ? simulation.Settings().foodFieldDecayPerSecond
            : simulation.Settings().homeFieldDecayPerSecond;
    }

    Entity FindNearestFood(Registry& registry, glm::vec2 position) const
    {
        Entity nearest(-1);
        float bestDistance = std::numeric_limits<float>::max();
        for (Entity entity : registry.GetAllEntities())
        {
            if (!entity.HasComponent<FoodSourceComponent>() ||
                !entity.HasComponent<TransformComponent>() ||
                GetFoodAmount(entity) <= 0)
            {
                continue;
            }

            const float distance = glm::length(GetEntityPosition(entity) - position);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                nearest = entity;
            }
        }
        return nearest;
    }

    Entity FindNearestColony(Registry& registry, glm::vec2 position) const
    {
        Entity nearest(-1);
        float bestDistance = std::numeric_limits<float>::max();
        for (Entity entity : registry.GetAllEntities())
        {
            if (!entity.HasComponent<AntColonyComponent>() || !entity.HasComponent<TransformComponent>())
            {
                continue;
            }

            const float distance = glm::length(GetEntityPosition(entity) - position);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                nearest = entity;
            }
        }
        return nearest;
    }

    static glm::vec2 GetEntityPosition(Entity entity)
    {
        return entity.GetComponent<TransformComponent>().position;
    }

    static int GetFoodAmount(Entity food)
    {
        if (food.HasComponent<AttributesComponent>())
        {
            return food.GetComponent<AttributesComponent>().values.value("food_amount", 0);
        }

        if (food.HasComponent<FoodSourceComponent>())
        {
            return food.GetComponent<FoodSourceComponent>().foodAmount;
        }

        return 0;
    }

    static bool TakeFood(Entity food)
    {
        if (food.HasComponent<AttributesComponent>())
        {
            auto& attributes = food.GetComponent<AttributesComponent>().values;
            const int amount = attributes.value("food_amount", 0);
            if (amount <= 0)
            {
                return false;
            }

            attributes["food_amount"] = amount - 1;
            return true;
        }

        if (food.HasComponent<FoodSourceComponent>())
        {
            auto& foodSource = food.GetComponent<FoodSourceComponent>();
            if (foodSource.foodAmount <= 0)
            {
                return false;
            }

            foodSource.foodAmount--;
            return true;
        }

        return false;
    }

    float SampleField(
        const AntSwarmSimulation& simulation,
        const std::string& fieldName,
        glm::vec2 worldPosition
    ) const
    {
        const auto found = simulation.GetFields().find(fieldName);
        if (found == simulation.GetFields().end())
        {
            return 0.0f;
        }

        const FieldGrid& fieldGrid = found->second;
        if (fieldGrid.cellWorldSize <= 0.0f || fieldGrid.rows <= 0 || fieldGrid.cols <= 0)
        {
            return 0.0f;
        }

        const int col = static_cast<int>(std::floor(worldPosition.x / fieldGrid.cellWorldSize));
        const int row = static_cast<int>(std::floor(worldPosition.y / fieldGrid.cellWorldSize));
        if (row < 0 || col < 0 || row >= fieldGrid.rows || col >= fieldGrid.cols)
        {
            return 0.0f;
        }

        const std::size_t index = static_cast<std::size_t>(row) *
            static_cast<std::size_t>(fieldGrid.cols) +
            static_cast<std::size_t>(col);
        return index < fieldGrid.values.size() ? static_cast<float>(fieldGrid.values[index]) : 0.0f;
    }

    float MarkerSamplingCoefficient(
        const AntSwarmSimulation& simulation,
        const TileMap& tileMap,
        glm::vec2 position
    )
    {
        const float probe = simulation.Settings().agentRadius * 2.0f;
        float coefficient = 1.0f;
        for (glm::vec2 direction : {
            glm::vec2(1.0f, 0.0f),
            glm::vec2(-1.0f, 0.0f),
            glm::vec2(0.0f, 1.0f),
            glm::vec2(0.0f, -1.0f)
        })
        {
            if (!CanOccupy(simulation, tileMap, position + direction * probe))
            {
                coefficient *= 0.25f;
            }
        }
        return coefficient;
    }

    bool CanOccupy(const AntSwarmSimulation& simulation, const TileMap& tileMap, glm::vec2 position) const
    {
        return GridRaycaster::CanOccupyTileMap(
            tileMap,
            position,
            simulation.Settings().agentRadius,
            [](TerrainType terrain)
            {
                return terrain != TerrainType::Land && terrain != TerrainType::Runway;
            }
        );
    }

    bool CanTravel(
        const AntSwarmSimulation& simulation,
        const TileMap& tileMap,
        glm::vec2 from,
        glm::vec2 to
    ) const
    {
        return GridRaycaster::CanTravelTileMap(
            tileMap,
            from,
            to,
            simulation.Settings().agentRadius,
            [](TerrainType terrain)
            {
                return terrain != TerrainType::Land && terrain != TerrainType::Runway;
            }
        );
    }

    void RepairInvalidPosition(AntSwarmSimulation& simulation, AntAgent& ant, const TileMap& tileMap)
    {
        if (CanOccupy(simulation, tileMap, ant.position))
        {
            return;
        }

        constexpr float Pi = 3.14159265358979323846f;
        for (int radiusStep = 1; radiusStep <= 10; radiusStep++)
        {
            const float radius = simulation.Settings().agentRadius * 2.0f * static_cast<float>(radiusStep);
            for (int sample = 0; sample < 32; sample++)
            {
                const float angle = static_cast<float>(sample) / 32.0f * Pi * 2.0f;
                const glm::vec2 direction(std::cos(angle), std::sin(angle));
                const glm::vec2 candidate = ant.position + direction * radius;
                if (CanOccupy(simulation, tileMap, candidate))
                {
                    ant.position = candidate;
                    ant.velocity = glm::vec2(0.0f);
                    ant.direction = direction;
                    ant.target = candidate + direction * simulation.Settings().sensorDistanceMin;
                    ant.distanceToTarget = simulation.Settings().sensorDistanceMin;
                    ant.stuckTime = 0.0;
                    return;
                }
            }
        }
    }

    glm::vec2 RandomOpenDirection(
        AntSwarmSimulation& simulation,
        const TileMap& tileMap,
        glm::vec2 position
    )
    {
        const float probeDistance = std::max(simulation.Settings().sensorDistanceMin, 12.0f);
        for (int attempt = 0; attempt < 32; attempt++)
        {
            glm::vec2 direction = RandomDirection(simulation);
            if (CanTravel(simulation, tileMap, position, position + direction * probeDistance))
            {
                return direction;
            }
        }

        return RandomDirection(simulation);
    }

    glm::vec2 RandomDirection(AntSwarmSimulation& simulation)
    {
        constexpr float Pi = 3.14159265358979323846f;
        const float angle = RandomRange(simulation, -Pi, Pi);
        return glm::vec2(std::cos(angle), std::sin(angle));
    }

    float Random01(AntSwarmSimulation& simulation)
    {
        return std::uniform_real_distribution<float>(0.0f, 1.0f)(simulation.RandomEngine());
    }

    float RandomRange(AntSwarmSimulation& simulation, float min, float max)
    {
        return std::uniform_real_distribution<float>(min, max)(simulation.RandomEngine());
    }

    static glm::vec2 TurnTowards(glm::vec2 current, glm::vec2 target, float maxRadians)
    {
        current = SafeNormalize(current, glm::vec2(1.0f, 0.0f));
        target = SafeNormalize(target, current);

        const float dot = std::clamp(glm::dot(current, target), -1.0f, 1.0f);
        const float angle = std::acos(dot);
        if (angle <= maxRadians || angle <= 0.0001f)
        {
            return target;
        }

        const float cross = current.x * target.y - current.y * target.x;
        const float turn = cross >= 0.0f ? maxRadians : -maxRadians;
        return Rotate(current, turn);
    }

    static glm::vec2 Rotate(glm::vec2 direction, float angle)
    {
        direction = SafeNormalize(direction, glm::vec2(1.0f, 0.0f));
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        return SafeNormalize(glm::vec2(
            direction.x * c - direction.y * s,
            direction.x * s + direction.y * c
        ), direction);
    }

    static glm::vec2 SafeNormalize(glm::vec2 value, glm::vec2 fallback)
    {
        const float length = glm::length(value);
        if (length <= 0.0001f)
        {
            const float fallbackLength = glm::length(fallback);
            return fallbackLength <= 0.0001f ? glm::vec2(1.0f, 0.0f) : fallback / fallbackLength;
        }

        return value / length;
    }
};

#endif // PIPEFRAME_ANT_SWARM_SIMULATION_SYSTEM_H
