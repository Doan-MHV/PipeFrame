#ifndef PIPEFRAME_SIMULATIONUPDATEPHASE_H
#define PIPEFRAME_SIMULATIONUPDATEPHASE_H

#include <array>

enum class SimulationUpdatePhase
{
    Input,
    ProjectSimulation,
    ProjectEntitySystems,
    Movement,
    TerrainConstraint,
    SoftCollision,
    Collision,
    ProjectileEmission,
    ProjectileLifecycle,
    OffscreenLifecycle,
    Animation,
    Camera
};

inline constexpr std::array<SimulationUpdatePhase, 12> PlaySimulationUpdateOrder = {
    SimulationUpdatePhase::Input,
    SimulationUpdatePhase::ProjectSimulation,
    SimulationUpdatePhase::ProjectEntitySystems,
    SimulationUpdatePhase::Movement,
    SimulationUpdatePhase::TerrainConstraint,
    SimulationUpdatePhase::SoftCollision,
    SimulationUpdatePhase::Collision,
    SimulationUpdatePhase::ProjectileEmission,
    SimulationUpdatePhase::ProjectileLifecycle,
    SimulationUpdatePhase::OffscreenLifecycle,
    SimulationUpdatePhase::Animation,
    SimulationUpdatePhase::Camera
};

#endif
