#ifndef ANT_CONTACT_SOLVER_H
#define ANT_CONTACT_SOLVER_H

#include <cstddef>
#include <PipeFrame/Simulation/System.h>
#include "World/Runtime/AntQuery.h"
#include "World/Physics/AntBodySystem.h"
#include "World/Runtime/Environment/AntEnvironment.h"

namespace ant_simulation {

struct ContactSolverResult {
    std::size_t antContacts{0};
    std::size_t wallConstraints{0};
};

class ContactSolver final : public pipeframe::PhysicsSolver<AntBodySystem, ContactSolverResult> {
public:
    static constexpr float ContactDistance{
        1.0f
    };

    ContactSolver(
        AntQuery &antStore,
        AntEnvironment &environment
    );

    [[nodiscard]] std::string_view GetSystemId() const override { return "ant.contact-solver"; }
    ContactSolverResult Solve(
        AntBodySystem &physicsWorld
    ) override;

private:
    bool SolveWallConstraint(
        AntPhysicsBody &body
    ) const;

    AntEnvironment &environment;
    pipeframe::CircleContactWorkspace contacts;
};

} // namespace ant_simulation

#endif
