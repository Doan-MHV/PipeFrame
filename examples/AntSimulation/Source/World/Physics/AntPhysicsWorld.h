#pragma once
#include "World/Physics/AntBodySystem.h"
#include "World/Physics/AntMovementSystem.h"
#include "World/Physics/ContactSolver.h"
#include <PipeFrame/World/World.h>
#include <PipeFrame/Render/WorldDebugView.h>
#include "World/Runtime/Environment/AntEnvironment.h"
namespace ant_simulation {
class AntPhysicsWorld final : public pipeframe::PhysicsWorld {
  public:
    AntPhysicsWorld(AntQuery &ants, AntEnvironment &environment, const AntConfiguration &configuration)
        : bodies(ants, configuration), contacts(ants, environment), movement(ants, bodies, contacts, configuration) {
        AddStep([this](float delta) { lastResult = movement.Update(delta); });
    }
    void CollectDebug(pipeframe::WorldDebugDraw &draw, const AntEnvironment &environment) const {
        for(const auto &body:bodies.GetBodies())draw.Circle(body.position,body.radius);
        const auto wall=[&](int x,int y){const auto *cell=environment.TryGetCell(x,y);return cell&&cell->wall;};
        for(int y=0;y<environment.GetHeight();++y)for(int x=0;x<environment.GetWidth();++x)if(wall(x,y)){
            const std::array<pipeframe::Vector2f,4> corners{{{float(x),float(y)},{float(x+1),float(y)},{float(x+1),float(y+1)},{float(x),float(y+1)}}};
            for(int edge=0;edge<4;++edge)if(!wall(x+(edge==1)-(edge==3),y+(edge==2)-(edge==0)))draw.Line(corners[edge],corners[(edge+1)%4]);
        }
    }
    AntBodySystem bodies;
    ContactSolver contacts;
    AntMovementSystem movement;
    AntMovementResult lastResult;
};
} // namespace ant_simulation
