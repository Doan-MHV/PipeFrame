#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include <PipeFrame/Backend/SFML/DashboardAccess.h>
#include "Runtime/AntSimulationRuntime.h"
#include "Runtime/AntTypeIds.h"
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <PipeFrame/Backend/SFML/Conversions.h>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <iostream>
#include <cstdlib>
#include <PipeFrame/Components/PlaygroundComponent.h>

void Require(bool value, const char *message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
TextButton *Find(Widget &root, const std::string &caption) {
    if (auto *button = dynamic_cast<TextButton *>(&root))
        for (std::size_t i = 0; i < button->GetChildCount(); ++i)
            if (auto *label = dynamic_cast<Label *>(button->GetChild(i)); label && label->GetText() == caption)
                return button;
    for (std::size_t i = 0; i < root.GetChildCount(); ++i)
        if (auto *button = Find(*root.GetChild(i), caption)) return button;
    return nullptr;
}
int main(int argc, char **argv) {
    { pipeframe::GraphicsResourceService resources; bool rejected=false;
      try { SimulationDashboard invalid(resources,{},"Invalid font"); }
      catch(const std::invalid_argument &) { rejected=true; }
      Require(rejected,"Neutral dashboard rejects an invalid font instead of dereferencing it"); }

    sf::RenderWindow window(sf::VideoMode({1000, 800}), "Ant rework baseline");
    window.setVisible(false);
    auto context = pipeframe::backend::sfml::MakeRenderContext(window);
    ant_simulation::AntSimulationRuntime ant;
    std::string error;
    Require(ant.Load({PIPEFRAME_ANT_SOURCE}, error), error.c_str());
    auto colony = ant.CreateDefaultObject(ant_simulation::ColonyTypeId);
    colony.id = 1;
    colony.properties[ant_simulation::InitialPopulationKey] = std::int64_t{4};
    colony.transform.position = {30, 30};
    auto ground=ant.CreateDefaultObject(pipeframe::PlaygroundEntityTypeId);ground.id=2;
    ant.SynchronizeScene(std::array{colony,ground});
    ant.Start();
    for (int i = 0; i < 20; ++i) ant.FixedUpdate(1.0f / 60.0f);
    auto render = [&] { context.BeginScreen(); ant.RenderScreen(context); };
    render();
    auto &dashboard = pipeframe::backend::sfml::DashboardAccess::Host(*ant.GetDashboard());
    const auto ants=ant.GetSimulationWorld()->GetAntQuery().GetAnts();
    Require(!ants.empty() && ant.SelectAntAt(ants.front().GetPosition(),2),"Select a real ECS ant for the preview");
    render();
    auto click = [&](Widget &widget) {
        const auto point = sf::Vector2i(widget.GetScreenPosition() + widget.GetSize() * 0.5f);
        Require(ant.ConsumesPointerAt(pipeframe::backend::sfml::FromBackend(point), context), "Visible control must own pointer input");
        Require(ant.HandleUIEvent(*pipeframe::backend::sfml::FromBackend(sf::Event(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, point})), context), "Press consumed");
        render();
        Require(ant.HandleUIEvent(*pipeframe::backend::sfml::FromBackend(sf::Event(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, point})), context), "Release consumed after refresh");
    };
    Require(dashboard.GetIndependentDrawerCount()==6,"All six Ant panels are present");
    const std::filesystem::path evidence=argc>2?argv[2]:"";
    if(!evidence.empty())std::filesystem::create_directories(evidence);
    for(const sf::Vector2u size: {sf::Vector2u{1000,800},sf::Vector2u{640,480}}) {
        context.GetCamera().SetViewport({{0,0},{float(size.x)/1000,float(size.y)/800}});
    for (std::size_t i = 0; i < dashboard.GetIndependentDrawerCount(); ++i) {
        for (std::size_t j = 0; j < dashboard.GetIndependentDrawerCount(); ++j)
            dashboard.GetIndependentDrawer(j).Close(false);
        render();
        auto &drawer = dashboard.GetIndependentDrawer(i);
        click(drawer.GetHandle());
        dashboard.Update(0.3f); render();
        Require(drawer.IsOpen(), "Drawer handle opens panel");
        if(i==0) {
            // The editor tool labels are project-owned; every mode must exist.
            Require(Find(drawer,"SELECT")!=nullptr,"Declarative selection tool exists");
            for(const auto mode:{ant_simulation::AntEditorToolMode::Erase,ant_simulation::AntEditorToolMode::AddFood,ant_simulation::AntEditorToolMode::AddWall}) {
                auto *tool=Find(drawer,ant_simulation::AntEditorTool::GetModeName(mode));Require(tool,"Environment tool is present");
                click(*tool);render();Require(ant.GetEditorTool().GetMode()==mode,"Tool action changes simulation editor mode");
            }
            click(*Find(drawer,"Add Food"));render();
            if(size.x==640)ant.Stop(); // Also exercise live editing while paused.
            auto *world=ant.GetSimulationWorld();const auto tick=world->GetStatistics().tick;
            const auto antsBefore=world->GetAntQuery().GetCount();
            ant.GetEditorTool().SetRadius(3);ant.GetEditorTool().SetFoodQuantity(19);
            auto &environment=world->GetEnvironment();
            pipeframe::Vector2f at{20.5f,20.5f};
            for(int y=5;y<environment.GetHeight()-5;++y){bool found=false;
                for(int x=5;x<environment.GetWidth()-5;++x)if(auto *cell=environment.TryGetCell(x,y);cell&&!cell->wall&&!cell->foodQuantity){at={float(x)+.5f,float(y)+.5f};found=true;break;}
                if(found)break;
            }
            const auto pixel=context.MapWorldToPixel(at);
            ant.HandleEvent({pipeframe::InputEventType::PointerPressed,pipeframe::PointerInput{pipeframe::PointerButton::Right,pixel}},context);
            ant.HandleEvent({pipeframe::InputEventType::PointerReleased,pipeframe::PointerInput{pipeframe::PointerButton::Right,pixel}},context);
            Require(environment.TryGetCell(int(at.x),int(at.y))->foodQuantity>0,"Live Add Food works in an authored Playground scene");
            Require(ant.GetSimulationWorld()==world&&world->GetStatistics().tick==tick&&world->GetAntQuery().GetCount()==antsBefore,
                    "Live food painting preserves world, simulation time and population");
            click(*Find(drawer,"SELECT"));render();
        }
        if(i==1) {
            auto *grid=Find(drawer,"GRID ON");
            if(!grid)grid=Find(drawer,"GRID OFF");
            Require(grid,"Render toggle is available");
            const bool wasOn=Find(drawer,"GRID ON")!=nullptr;
            click(*grid); render();
            Require(Find(drawer,wasOn?"GRID OFF":"GRID ON"),"Toggle updates simulation options and view state");
        }
        if(i==3) {
            if(auto *expanded=Find(drawer,"LIVE COMPONENTS ON")){click(*expanded);render();}
            const bool followed=ant.GetAntInspectorData().follow;
            auto *follow=Find(drawer,followed?"FOLLOW ON":"FOLLOW OFF");Require(follow,"Selected ant actions are available");
            click(*follow);render();Require(ant.GetAntInspectorData().follow!=followed,"Follow action updates selected ant state");
            auto *components=Find(drawer,"LIVE COMPONENTS OFF");Require(components,"Live component disclosure exists");
            click(*components);render();Require(Find(drawer,"LIVE COMPONENTS ON"),"Selected ant exposes its real registered components");
        }
        if(i==5) {
            auto *button=Find(drawer,"PLAY");const bool paused=button!=nullptr;
            if(!button)button=Find(drawer,"PAUSE");Require(button,"Simulation control exists");
            click(*button);render();Require(Find(drawer,paused?"PAUSE":"PLAY"),"Simulation control publishes playing state");
        }
        if(!evidence.empty()) {
            sf::RenderTexture target(size); target.clear(sf::Color{18,20,24});
            dashboard.Render(target);target.display();
            Require(target.getTexture().copyToImage().saveToFile(evidence/("ant-"+std::to_string(size.x)+"-panel-"+std::to_string(i)+".png")),"Save Ant panel evidence");
        }
        auto *close = Find(drawer, "CLOSE");
        Require(close, "Panel has close action");
        click(*close);
        dashboard.Update(0.3f); render();
        Require(!drawer.IsOpen(), "Close returns to collapsed handle");
    }
    }
    context.GetCamera().SetViewport({{0,0},{1,1}}); render();
    if (argc > 1) {
        sf::RenderTexture target({1000, 800});
        target.clear(sf::Color(18, 20, 24));
        dashboard.Render(target); target.display();
        Require(target.getTexture().copyToImage().saveToFile(argv[1]), "Save baseline screenshot");
    }
    ant.Stop(); ant.Unload();
    std::cout << "Ant dashboard lifecycle and pointer capture passed\n";
}
