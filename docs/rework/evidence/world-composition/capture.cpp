#include "Runtime/AntSimulationRuntime.h"
#include "Runtime/AntTypeIds.h"
#include <PipeFrame/Render/RenderContext.h>
#include <SFML/Graphics/Image.hpp>
#include <cassert>
#include <iostream>
int main(){
 sf::RenderWindow window(sf::VideoMode({1000,800}),"World verification");window.setVisible(false);
 sf::RenderTexture target({1000,800});RenderContext context(target);
 ant_simulation::AntSimulationRuntime runtime;std::string error;
 if(!runtime.Load({"/Users/donvo/PipeFrame/examples/AntSimulation"},error)){std::cerr<<error;return 1;}
 auto colony=runtime.CreateDefaultObject(ant_simulation::ColonyTypeId);colony.id=1;colony.transform.position={96,108};
 colony.properties[ant_simulation::InitialPopulationKey]=std::int64_t{1000};runtime.SynchronizeScene(std::array{colony});
 assert(dynamic_cast<ant_simulation::AntRenderingWorld*>(runtime.GetSimulationWorld()->GetRenderingWorld()));
 runtime.Start();for(int i=0;i<120;++i)runtime.FixedUpdate(1.0f/60.0f);
 context.BeginWorld();runtime.Render(context);
 target.clear();context.BeginWorld();runtime.Render(context);context.BeginScreen();runtime.RenderScreen(context);target.display();
 if(!target.getTexture().copyToImage().saveToFile("/Users/donvo/PipeFrame/docs/rework/evidence/world-composition/ant-world.png"))return 2;
 std::cout<<"Rendered AntWorld-owned layers with "<<runtime.GetSimulationWorld()->GetAntQuery().GetCount()<<" ants\n";
 runtime.Stop();runtime.Unload();
}
