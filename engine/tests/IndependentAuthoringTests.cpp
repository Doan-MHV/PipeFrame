#include "../../docs/tutorials/independent-ui/CounterProject.h"
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <iostream>
#include <stdexcept>
using namespace pipeframe;
using namespace pipeframe::ui;
void Check(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
Widget *Find(Widget *root,const std::string &key){
    if(root->GetKey()==key)return root;
    for(std::size_t i=0;i<root->GetChildCount();++i)if(auto *child=Find(root->GetChild(i),key))return child;
    return nullptr;
}
int main(int argc,char **argv){try{
    counter_example::CounterRuntime runtime;
    auto data=runtime.CreateDefaultObject("counter.entity");data.id=1;
    runtime.SynchronizeScene(std::vector<SceneObjectData>{data});
    auto object=runtime.ResolveSceneObject(1);
    Check(object.IsValid() && runtime.InspectObjectComponents(1)->size()==2,"Registered components exposed");
    runtime.Start();runtime.FixedUpdate(.5f);
    Check(object.GetComponent<Transform2DComponent>()->position.x==.5f,"Attached behaviour executes");
    counter_example::CounterPanel panel(object);
    UIManager ui;Check(ui.LoadDefaultFont(PIPEFRAME_TEST_FONT),"Font loads in backend host");
    auto mount=ui.MountView(panel.DescribeView());mount.SetBounds(0,0,320,240);ui.Update(0);
    auto *button=Find(ui.GetRoot(0),"faster");Check(button,"Nested stateful control mounted");
    const auto position=button->GetScreenPosition()+sf::Vector2f{5,5};
    const sf::Vector2i point{int(position.x),int(position.y)};
    ui.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point});
    ui.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,point});ui.Update(0);
    Check(panel.Changes()==1 && object.GetComponent<counter_example::CounterComponent>()->rate==2,"UI edits actual attached data");
    sf::RenderTexture rendered({320,240}); rendered.clear(sf::Color{24,25,28});
    ui.Render(rendered); rendered.display();
    if(argc>1)Check(rendered.getTexture().copyToImage().saveToFile(argv[1]),"Save independent panel evidence");
    runtime.FixedUpdate(.5f);
    Check(object.GetComponent<Transform2DComponent>()->position.x==1.5f,"Behaviour consumes UI change");
    mount.SetBounds(0,0,180,90);ui.Update(0);
    Check(Find(ui.GetRoot(0),"faster")==button && panel.Changes()==1,"Resize retains state and keyed control");
    mount.Unmount();ui.Update(0);Check(!mount.IsMounted(),"Unmount disposes UI");
    runtime.Stop();runtime.Reset();
    Check(!object.IsValid(),"Reset invalidates old scene handle");
    Check(runtime.ResolveSceneObject(1).GetComponent<counter_example::CounterComponent>()->rate==1,"Reset restores authored data");
    runtime.Unload();
    std::cout<<"Independent registered component -> attached Behaviour -> nested stateful UI -> resize/unmount/reset passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
