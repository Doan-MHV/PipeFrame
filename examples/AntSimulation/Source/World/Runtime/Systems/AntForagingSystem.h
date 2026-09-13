#ifndef ANT_BEHAVIOR_SYSTEM_H
#define ANT_BEHAVIOR_SYSTEM_H
#include <random>
#include <PipeFrame/Simulation/System.h>
#include "World/Runtime/AntQuery.h"
#include "World/Runtime/Systems/ColonyLifecycleSystem.h"
#include "World/Runtime/Systems/WorkerBehavior.h"
namespace ant_simulation {
class AntForagingSystem final : public pipeframe::FixedUpdateSystem<void>{
public:AntForagingSystem(AntQuery&,ColonyLifecycleSystem&,AntEnvironment&,const AntConfiguration&,std::uint32_t seed);
[[nodiscard]]std::string_view GetSystemId()const override{return "ant.behavior";}void Update(float deltaTime)override;
private:AntQuery &store;ColonyLifecycleSystem &colonies;WorkerBehavior worker;std::mt19937 random;
};}
#endif
