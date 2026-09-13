#include "World/AntWorld.h"
#include <iostream>
int main(){ant_simulation::AntConfiguration config;ant_simulation::AntWorld world(config,1);std::string error;
 if(!world.Initialize(error)){std::cerr<<error;return 1;}
 world.CreateColony(1,{96,108},{239,71,111});world.CreateColony(2,{208,80},{80,200,100});world.CreateColony(3,{320,108},{50,70,220});
 for(int i=0;i<1200;++i)world.FixedUpdate(1.f/60.f);
 auto snapshot=world.CaptureBehaviorCheckpoint();std::cout<<snapshot.stateSignature<<" "<<snapshot.antCount<<" "<<snapshot.totalBirths<<" "<<snapshot.totalDeaths<<'\n';
}
