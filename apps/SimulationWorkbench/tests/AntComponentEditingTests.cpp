#include <PipeFrame/Backend/SFML/ViewPanelHost.h>
#include "../Editor/ProjectSession.h"
#include "../Editor/SceneSerializer.h"
#include "../Editor/InspectorPanel.h"
#include "../Runtime/SimulationSession.h"
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <PipeFrame/Backend/SFML/UI/LabeledNumericField.h>
#include <PipeFrame/Backend/SFML/UI/LabeledTextField.h>
#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/TextButton.h>
#include <PipeFrame/Backend/SFML/UI/ScrollPanel.h>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <cmath>
using namespace pipeframe;
using namespace pipeframe::editor;
namespace {
void Check(bool value,const std::string &message) { if (!value) throw std::runtime_error(message); }
PropertyValue Read(ProjectSession &project,SceneObjectId id,const std::string &type,const std::string &key) {
    const auto components=project.GetRuntime().InspectObjectComponents(id);
    Check(components.has_value(),"Runtime entity must resolve");
    const auto component=std::ranges::find(*components,type,&SceneComponentData::typeId);
    Check(component!=components->end(),"Attached component must be discoverable: "+type);
    return component->properties.at(key);
}
TextButton *FindButton(Widget &root,const std::string &text) {
    if(auto *button=dynamic_cast<TextButton *>(&root))for(std::size_t i=0;i<button->GetChildCount();++i)
        if(auto *label=dynamic_cast<Label *>(button->GetChild(i));label && label->GetText()==text)return button;
    for(std::size_t i=0;i<root.GetChildCount();++i)if(auto *result=FindButton(*root.GetChild(i),text))return result;
    return nullptr;
}
ScrollPanel *FindScroll(Widget &widget) {
    if (auto *scroll = dynamic_cast<ScrollPanel *>(&widget)) return scroll;
    for (std::size_t i = 0; i < widget.GetChildCount(); ++i)
        if (auto *scroll = FindScroll(*widget.GetChild(i))) return scroll;
    return nullptr;
}
template<class Row,class Input> Input *FindLabeled(Widget &widget,const std::string &caption) {
    if (auto *row=dynamic_cast<Row *>(&widget)) {
        bool matches=false; Input *field{};
        for(std::size_t i=0;i<row->GetChildCount();++i) {
            if(auto *label=dynamic_cast<Label *>(row->GetChild(i))) matches |= label->GetText().starts_with(caption);
            if(auto *value=dynamic_cast<Input *>(row->GetChild(i))) field=value;
        }
        if(matches) return field;
    }
    for(std::size_t i=0;i<widget.GetChildCount();++i) if(auto *found=FindLabeled<Row,Input>(*widget.GetChild(i),caption)) return found;
    return nullptr;
}
}
int main() {
    const auto root=std::filesystem::temp_directory_path()/("pipeframe-r2-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        std::filesystem::create_directories(root/"Scenes"); std::filesystem::create_directories(root/"Build");
        std::filesystem::copy(PIPEFRAME_ANT_ASSETS,root/"Assets",std::filesystem::copy_options::recursive);
        const auto library=std::filesystem::path(PIPEFRAME_ANT_RUNTIME);
        const auto copy=root/"Build"/library.filename(); std::filesystem::copy_file(library,copy);
        {
            std::ofstream manifest(root/"project.pipeframe");
            manifest << "PIPEFRAME_PROJECT 2\n\"R2 acceptance\"\n\"Scenes/Main.pfscene\"\n" << std::quoted((std::filesystem::path("Build")/library.filename()).generic_string()) << '\n';
        }
        SceneDocument initial;
        const auto first=initial.CreateObject("First","ant.colony",{{96,108},0,{1,1}},{{"initialPopulation",std::int64_t{10}},{"movementSpeed",2.0}});
        const auto second=initial.CreateObject("Second","ant.colony",{{196,108},0,{1,1}},{{"initialPopulation",std::int64_t{20}},{"movementSpeed",3.0}});
        std::string error;
        Check(SceneSerializer::Save(initial,root/"Scenes/Main.pfscene",&error),error);
        ProjectSession project(root/"recent"); Check(project.OpenProject(root/"project.pipeframe",&error),error);
        Check(project.GetRuntime().HasRuntime(),"Real Ant runtime loads"); project.SetSelectedObject(first);
        Check(std::get<double>(Read(project,first,"ant.colony","movementSpeed"))==2 &&
              std::get<double>(Read(project,second,"ant.colony","movementSpeed"))==3,"Colonies keep independent typed settings");
        UIManager ui; Check(ui.LoadDefaultFont(PIPEFRAME_TEST_FONT),"Font loads");
        auto &panel=ui.CreateRoot<pipeframe::backend::sfml::HostedViewPanel<InspectorPanel>>(ui.GetDefaultFont()); panel.SetSize({420,1300}); panel.SetAuthoringEnabled(true);
        const auto refresh=[&] {
            const auto *selected=project.GetSelectedObject();
            auto live=*selected; live.components=*project.GetRuntime().InspectObjectComponents(selected->id);
            panel.SetLiveObjects({live}); panel.SetLiveComponentProperties(project.GetRuntime().GetLiveComponentProperties());
            const SceneObjectData *selectedList[]{selected};
            panel.SetSelection(selectedList,project.FindObjectType(selected->typeId),project.GetComponentTypes()); panel.SetAuthoringEnabled(true); ui.Update(0);
        };
        bool committed=false;
        panel.SetOnComponentPropertyCommitted([&](const auto &type,const auto &key,const auto &value) { committed=project.SetSelectedComponentProperty(type,key,value); });
        refresh();
        const auto sections=panel.GetVisibleComponentSections();
        Check(std::ranges::any_of(sections,[](const auto &name) { return name.find("Colony State")!=std::string::npos; }),"Inspector discovers live component absent from scene file");
        auto *field=FindLabeled<LabeledNumericField,NumericField>(panel,"Ant Speed"); Check(field,"Schema-generated editor field exists");
        const auto position=field->GetScreenPosition()+sf::Vector2f{5,5}; const sf::Vector2i point{int(position.x),int(position.y)};
        ui.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point}); ui.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,point});
        ui.HandleEvent(sf::Event::TextEntered{U'6'}); refresh(); ui.Update(0);
        ui.HandleEvent(sf::Event::TextEntered{U'.'}); ui.HandleEvent(sf::Event::TextEntered{U'5'});
        ui.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Enter});
        Check(committed && std::get<double>(Read(project,first,"ant.colony","movementSpeed"))==6.5,"Real Inspector input updates actual colony ECS data");
        Check(std::get<double>(Read(project,second,"ant.colony","movementSpeed"))==3,"Edit leaves other colony unchanged");
        refresh();
        auto *seed=FindLabeled<LabeledTextField,TextField>(panel,"Random Seed"); Check(seed,"Integer property uses exact text input");
        const auto seedPosition=seed->GetScreenPosition()+sf::Vector2f{5,5}; const sf::Vector2i seedPoint{int(seedPosition.x),int(seedPosition.y)};
        ui.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,seedPoint}); ui.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,seedPoint});
        for (const auto digit : std::string("4294967295")) ui.HandleEvent(sf::Event::TextEntered{static_cast<char32_t>(digit)});
        ui.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Enter});
        Check(std::get<std::int64_t>(Read(project,first,"ant.colony","randomSeed"))==4294967295LL,"Inspector preserves full integer precision");
        Check(project.SetSelectedComponentProperty(Transform2DComponentTypeId,"position",Vector2f{120,140}),"Transform command commits");
        Check(project.SetSelectedComponentProperty(Transform2DComponentTypeId,"rotation",90.0),"Rotation command commits");
        Check(std::abs(std::get<double>(Read(project,first,Transform2DComponentTypeId,"rotation"))-90)<0.001,"Live Transform reports degrees");
        Check(!project.SetSelectedComponentProperty("ant.colony","movementSpeed",-1.0) &&
              !project.SetSelectedComponentProperty("ant.colony-state","reserve",50.0) &&
              !project.RemoveSelectedComponent("ant.colony"),"Invalid/read-only edits and required removal rejected");
        SimulationSession simulation(project.GetRuntime()); simulation.Toggle(project.GetDocument().GetObjects());
        simulation.FixedUpdate(1.0f/60); simulation.Toggle(project.GetDocument().GetObjects());
        const auto members=Read(project,first,"ant.colony-state","members");
        const auto reserve=Read(project,first,"ant.colony-state","reserve");
        Check(std::get<std::int64_t>(members)>0,"Simulation uses authored population");
        Check(project.SetSelectedComponentProperty("ant.colony","spawnRadius",7.0),"Paused edit succeeds");
        Check(Read(project,first,"ant.colony-state","members")==members && Read(project,first,"ant.colony-state","reserve")==reserve,"Paused edit preserves running state");
        Check(project.Undo() && std::get<double>(Read(project,first,"ant.colony","spawnRadius"))==4,"Undo edits actual component");
        Check(Read(project,first,"ant.colony-state","members")==members,"Undo does not rebuild/reset simulation");
        Check(project.Redo() && std::get<double>(Read(project,first,"ant.colony","spawnRadius"))==7,"Redo edits actual component");
        Check(project.SetSelectedComponentProperty("ant.colony","initialPopulation",std::int64_t{25}),"Preview population edit authors next reset");
        Check(Read(project,first,"ant.colony-state","reserve")==reserve,"Initial population does not retroactively spawn during preview");
        Check(project.Save(&error),error);
        const auto saved=project.GetDocument().GetObjects();
        for(const auto &object:saved) Check(std::ranges::none_of(object.components,[](const auto &c) { return c.typeId=="ant.colony-state"; }),"Telemetry is not serialized into authored scene");
        simulation.Reset(project.GetDocument().GetObjects());
        Check(std::get<std::int64_t>(Read(project,first,"ant.colony-state","members"))==0,"Reset clears preview population");
        Check(std::get<double>(Read(project,first,"ant.colony-state","reserve"))>std::get<double>(reserve),"Reset uses edited initial population");
        Check(project.OpenProject(root/"project.pipeframe",&error),error); project.SetSelectedObject(first);
        Check(std::get<double>(Read(project,first,"ant.colony","movementSpeed"))==6.5 &&
              std::get<std::int64_t>(Read(project,first,"ant.colony","initialPopulation"))==25 &&
              std::get<Vector2f>(Read(project,first,Transform2DComponentTypeId,"position"))==Vector2f{120,140},"Save/reopen restores actual typed components");
        refresh(); sf::RenderTexture target({420,1300}); target.clear(sf::Color{24,25,28}); ui.Render(target); target.display();
        Check(target.getTexture().copyToImage().saveToFile("r2-live-inspector.png"),"Save real Inspector evidence");
        panel.SetSize({420, 550}); refresh();
        auto *scroll = FindScroll(panel); Check(scroll, "Inspector owns shared scroll container");
        Check(scroll->GetMaximumScrollOffset() > 500, "Short Inspector has overflowing content");
        const auto wheelPosition = scroll->GetScreenPosition() + sf::Vector2f{100, 100};
        const sf::Vector2i wheelPoint{int(wheelPosition.x), int(wheelPosition.y)};
        ui.HandleEvent(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical, -5, wheelPoint});
        const float offset = scroll->GetScrollOffset();
        Check(offset > 0, "Wheel over Inspector child scrolls its container");
        for (int i = 0; i < 10; ++i) { refresh(); ui.Update(0); }
        Check(std::abs(scroll->GetScrollOffset() - offset) < .01f, "Live refresh retains scroll position");
        ui.HandleEvent(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical, -1000, wheelPoint});
        refresh();
        Check(scroll->GetScrollOffset() == scroll->GetMaximumScrollOffset(), "Bottom remains reachable after refresh");
        sf::RenderTexture shortTarget({420,550}); shortTarget.clear(sf::Color{24,25,28}); ui.Render(shortTarget); shortTarget.display();
        Check(shortTarget.getTexture().copyToImage().saveToFile("r2-scrolled-inspector.png"), "Save short scrolled Inspector evidence");
        panel.SetSize({420,700}); refresh();
        Check(scroll->GetScrollOffset() <= scroll->GetMaximumScrollOffset(), "Resize clamps to final content bounds");
        project.SetSelectedObject(second); refresh();
        scroll=FindScroll(panel); Check(scroll && scroll->GetScrollOffset() == 0, "New selection starts at top");
        // R6: a project entity beyond ants/colonies uses the same creation/schema/lifecycle path.
        simulation.Stop();
        const auto countBefore=project.GetDocument().GetObjects().size();
        Check(!project.CreateObjectOfType("unknown.entity"),"Unknown creation type rejects without mutation");
        Check(project.CreateObjectOfType("ant.signal-beacon",Vector2f{160,140}),"Create registered Signal Beacon");
        const auto beacon=project.GetSelectedObject()->id;
        Check(project.GetSelectedObject()->typeId=="ant.signal-beacon","Explicit type overrides selected colony");
        Check(project.Undo() && project.GetDocument().GetObjects().size()==countBefore,"Undo entity creation");
        Check(project.Redo(),"Redo entity creation"); project.SetSelectedObject(beacon);
        Check(project.SetSelectedComponentProperty("ant.signal-beacon","rotationSpeed",180.0),"Edit beacon behaviour settings");
        Check(project.SetSelectedComponentProperty("ant.signal-beacon","radius",8.0),"Edit beacon geometry");
        Check(!project.SetSelectedComponentProperty("ant.signal-beacon","radius",-1.0),"Beacon schema rejects invalid radius");
        Check(std::get<double>(Read(project,beacon,"ant.signal-beacon","radius"))==8.0,"Rejected edit leaves ECS value intact");
        simulation.Toggle(project.GetDocument().GetObjects()); simulation.FixedUpdate(1.f/60.f);
        Check(std::abs(std::get<double>(Read(project,beacon,Transform2DComponentTypeId,"rotation"))-3.0)<.01,"Beacon Behaviour rotates actual ECS Transform");
        simulation.Toggle(project.GetDocument().GetObjects()); simulation.FixedUpdate(1.f/60.f);
        Check(std::abs(std::get<double>(Read(project,beacon,Transform2DComponentTypeId,"rotation"))-3.0)<.01,"Pause stops beacon behaviour");
        simulation.Reset(project.GetDocument().GetObjects());
        Check(std::abs(std::get<double>(Read(project,beacon,Transform2DComponentTypeId,"rotation")))<.01,"Reset restores authored transform");
        Check(project.Save(&error),error);
        Check(project.OpenProject(root/"project.pipeframe",&error),error);
        Check(std::get<double>(Read(project,beacon,"ant.signal-beacon","radius"))==8.0 &&
              std::get<double>(Read(project,beacon,"ant.signal-beacon","rotationSpeed"))==180.0,"Beacon components survive save/reopen");
        project.SetSelectedObject(beacon); panel.SetSize({420,1600});
        panel.SetOnComponentAttachment([&](const std::string &id,bool add) {
            const auto *type=project.FindComponentType(id);Check(type,"Registered attachment schema exists");
            SceneComponentData value;value.typeId=id;value.schemaVersion=type->schemaVersion;
            for(const auto &field:type->properties)value.properties.emplace(field.key,field.defaultValue);
            Check(add?project.AddSelectedComponent(value):project.RemoveSelectedComponent(id),"Inspector attachment command succeeds");
        });
        refresh();
        const auto clickButton=[&](const char *text) {
            auto *button=FindButton(panel,text);Check(button,std::string("Inspector action exists: ")+text);
            const auto p=button->GetScreenPosition()+button->GetSize()*.5f;const sf::Vector2i point{int(p.x),int(p.y)};
            ui.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point});
            ui.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,point});refresh();
        };
        clickButton("ADD COMPONENT / BEHAVIOUR");clickButton("ADD Motion");
        Check(std::get<double>(Read(project,beacon,"pipeframe.motion2d","speed"))==0,"Inspector attaches actual engine component");
        Check(!FindButton(panel,"REMOVE Transform"),"Required Transform is protected");
        clickButton("REMOVE Motion");
        const auto attached=project.GetRuntime().InspectObjectComponents(beacon);
        Check(std::ranges::none_of(*attached,[](const auto &c){return c.typeId=="pipeframe.motion2d";}),"Inspector detaches component from live ECS");
        project.GetRuntime().Unload(); std::filesystem::remove_all(root);
        std::cout << "R2 live ECS Inspector, colony isolation, undo/redo, play/reset and save/reopen passed.\n";
    } catch(const std::exception &error) { std::cerr << "FAILED: " << error.what() << "\nFixture: " << root << '\n'; return 1; }
}
