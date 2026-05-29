#ifndef PIPEFRAME_SIMULATIONUPDATEPHASE_H
#define PIPEFRAME_SIMULATIONUPDATEPHASE_H

#include <array>

enum class SimulationUpdatePhase
{
    Input,
    ProjectSimulation,
    Movement,
    SoftCollision,
    Collision,
    ProjectileEmission,
    ProjectileLifecycle,
    Animation,
    Camera
};

inline constexpr std::array<SimulationUpdatePhase, 9> PlaySimulationUpdateOrder = {
    SimulationUpdatePhase::Input,
    SimulationUpdatePhase::ProjectSimulation,
    SimulationUpdatePhase::Movement,
    SimulationUpdatePhase::SoftCollision,
    SimulationUpdatePhase::Collision,
    SimulationUpdatePhase::ProjectileEmission,
    SimulationUpdatePhase::ProjectileLifecycle,
    SimulationUpdatePhase::Animation,
    SimulationUpdatePhase::Camera
};

#endif
