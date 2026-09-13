#pragma once
#include <PipeFrame/World/World.h>
#include "World/Runtime/AntQuery.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Systems/ColonyLifecycleSystem.h"
#include "World/Runtime/Systems/AntForagingSystem.h"
#include "World/Runtime/Systems/AntCleanupSystem.h"
namespace ant_simulation {
class AntRuntimeWorld final : public pipeframe::RuntimeWorld {
public:
    AntRuntimeWorld(const AntConfiguration &configuration,std::uint32_t seed)
        :ants(GetScene()),colonies(environment,ants,configuration,seed),foraging(ants,colonies,environment,configuration,seed) {
        AddPreSceneSystem(environment);
        AddSystem(colonies);
    }
    void BindPhysics(AntBodySystem &bodies,const AntConfiguration &configuration) {
        cleanup=std::make_unique<AntCleanupSystem>(ants,colonies,bodies,configuration.worldSize);
    }
    AntEnvironment environment;
    AntQuery ants;
    ColonyLifecycleSystem colonies;
    AntForagingSystem foraging;
    std::unique_ptr<AntCleanupSystem> cleanup;
};
}
