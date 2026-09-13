#pragma once
#include <PipeFrame/World/World.h>
#include "World/Physics/AntBodySystem.h"
#include "World/Physics/ContactSolver.h"
#include "World/Physics/AntMovementSystem.h"
namespace ant_simulation {
class AntPhysicsWorld final : public pipeframe::PhysicsWorld {
public:
    AntPhysicsWorld(AntQuery &ants,AntEnvironment &environment,const AntConfiguration &configuration)
        :bodies(ants,configuration),contacts(ants,environment),movement(ants,bodies,contacts,configuration) {
        AddStep([this](float delta){lastResult=movement.Update(delta);});
    }
    AntBodySystem bodies;
    ContactSolver contacts;
    AntMovementSystem movement;
    AntMovementResult lastResult;
};
}
