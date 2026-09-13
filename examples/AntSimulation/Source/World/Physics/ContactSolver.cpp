#include "World/Physics/ContactSolver.h"

#include <PipeFrame/Physics/Physics2D.h>
#include <PipeFrame/Physics/GridCollision2D.h>

#include "World/Runtime/Environment/AntWorldCell.h"

namespace ant_simulation {

ContactSolver::ContactSolver(AntQuery &antStore,AntEnvironment &newEnvironment)
    : environment(newEnvironment) { (void)antStore; }

ContactSolverResult ContactSolver::Solve(AntBodySystem &physicsWorld) {
    ContactSolverResult result;
    // Both solvers consume body storage only. Publish the final solved positions
    // once, after all constraints; intermediate ECS writes have no reader.
    for(AntPhysicsBody &body:physicsWorld.GetBodies()) if(SolveWallConstraint(body)) ++result.wallConstraints;

    auto bodies=physicsWorld.GetBodies();
    result.antContacts=pipeframe::SolveCircleContacts<AntPhysicsBody>(
        bodies,{{0.0f,0.0f},{static_cast<float>(environment.GetWidth()),static_cast<float>(environment.GetHeight())}},
        contacts, [](const pipeframe::Contact2D&){});
    physicsWorld.SynchronizeAntPositions();
    return result;
}

bool ContactSolver::SolveWallConstraint(AntPhysicsBody &body) const {
    const auto start=body.position-body.lastMove;
    const auto motion=pipeframe::MoveCircle({start,body.radius},body.lastMove,body.velocity,[&](auto circle,auto delta)->std::optional<pipeframe::ShapeHit2D>{
        const auto hit=pipeframe::SweepCircleGrid(circle,delta,{},1,environment.GetWidth(),environment.GetHeight(),[&](auto cell){
            const auto *value=environment.TryGetCell(cell.column,cell.row);return value&&value->wall;
        });
        return hit?std::optional{hit->contact}:std::nullopt;
    });
    if(!motion.collided)return false;
    body.position=motion.position;body.previousPosition=motion.position;
    body.lastMove=motion.position-start;body.velocity=motion.velocity;
    return true;
}

} // namespace ant_simulation
