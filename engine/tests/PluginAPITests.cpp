#include <PipeFrame/Project/PluginAPI.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool condition,const char *message){if(!condition){std::cerr<<"FAILED: "<<message<<'\n';std::exit(1);}}
class Scene final : public pipeframe::ComponentQuery {
public:
    std::span<pipeframe::SceneObjectData> Objects() override{return objects;}
    std::span<const pipeframe::SceneObjectData> Objects()const override{return objects;}
    std::vector<pipeframe::SceneObjectData> objects;
};
}

int main(){
    using namespace pipeframe;
    ExtensionRegistry extensions;ActionRegistry actions;SystemRegistry systems;
    PluginRegistrar plugin("sample.plugin",extensions,actions,systems);
    std::string error;
    int extensionRuns=0;
    for(const auto point:{ExtensionPoint::Panel,ExtensionPoint::Drawer,ExtensionPoint::Tool,
            ExtensionPoint::Gizmo,ExtensionPoint::Menu,ExtensionPoint::Command,ExtensionPoint::Overlay,
            ExtensionPoint::Importer,ExtensionPoint::Setting,ExtensionPoint::PartCategory,
            ExtensionPoint::AttachmentCompatibility,ExtensionPoint::ConnectionType,
            ExtensionPoint::EnvironmentBrush,ExtensionPoint::ValidationRule,
            ExtensionPoint::TelemetryStream,ExtensionPoint::SimulationDebugOverlay}){
        const auto index=extensions.All().size();
        ExtensionDescriptor extension{"sample.extension."+std::to_string(index),{},"Sample Extension",point};
        extension.invoke=[&](const auto &){++extensionRuns;};
        Require(plugin.Extension(std::move(extension)),
                "Every editor and simulation extension point should register.");
    }
    Require(extensions.All().size()==16&&extensions.Validate().empty(),"Extension registration should be complete and valid.");
    for(const auto &extension:extensions.All())Require(extensions.Invoke(extension.id,{},&error),"Every extension family should execute.");
    Require(extensionRuns==16,"Every extension callback should run through the registry boundary.");
    ExtensionDescriptor duplicate{"sample.extension.0",{},"Duplicate",ExtensionPoint::Panel};duplicate.invoke=[](const auto &){};
    Require(!plugin.Extension(std::move(duplicate),&error),"Duplicate extension IDs should fail.");
    ExtensionDescriptor throwing{"sample.throwing",{},"Throwing",ExtensionPoint::Tool};
    throwing.invoke=[](const auto &){throw std::runtime_error("extension failure");};
    Require(plugin.Extension(std::move(throwing),&error)&&!extensions.Invoke("sample.throwing",{},&error)&&
                error.find("sample.throwing")!=std::string::npos,
            "Extension callback failures should be isolated and attributed.");

    int actionRuns=0;
    Require(plugin.Action({"sample.focus",{},"Focus Selection","Scene",{"scene"},"F",
                           [&](const auto &){++actionRuns;}}),"An editor action should register.");
    Require(plugin.Action({"sample.find",{},"Find Asset","Assets",{"assets"},"F",
                           [&](const auto &){++actionRuns;}}),"The same shortcut may exist in a separate context.");
    Require(actions.Conflicts().empty()&&actions.Search("focus","scene").size()==1&&actions.Invoke("sample.focus")&&actionRuns==1,
            "Command search, contexts, and invocation should work.");
    Require(actions.Rebind("sample.find","F",&error),"Existing separate-context binding should remain valid.");
    Require(plugin.Action({"sample.other",{},"Other","Scene",{"scene"},"G",[](const auto &){} }),"A second scene action should register.");
    Require(!actions.Rebind("sample.other","F",&error),"Conflicting shortcuts in one context should be rejected.");
    Require(plugin.Action({"sample.throwing-action",{},"Throwing Action","Scene",{"scene"},"",
                           [](const auto &){throw std::runtime_error("action failure");}},&error)&&
                !actions.Invoke("sample.throwing-action",{},&error)&&error.find("sample.throwing-action")!=std::string::npos,
            "Action callback failures should be isolated and attributed.");

    std::vector<std::string> order;
    Require(plugin.System({"sample.prepare",{},SystemPhase::FixedPrePhysics,{}, {},{"pose"},false,false,false,
                           [&](SystemContext &context){order.push_back("prepare");context.jobs->Submit(2,[&]{order.push_back("job2");});context.jobs->Submit(1,[&]{order.push_back("job1");});}}),"Prepare system should register.");
    Require(plugin.System({"sample.physics",{},SystemPhase::FixedPhysics,{"sample.prepare"},{"pose"},{"velocity"},false,false,false,
                           [&](SystemContext &context){order.push_back("physics");context.commands->Destroy(7);context.events->Publish({"contact","7"});}}),"Physics system should register.");
    Require(plugin.System({"sample.behavior",{},SystemPhase::FixedBehavior,{"sample.physics"},{"velocity"},{"intent"},true,false,false,
                           [&](SystemContext &context){order.push_back("behavior");Require(context.random->NextU32()!=0,"Deterministic random should be available.");}}),"Behavior system should register.");
    Require(systems.Validate().empty(),"A valid dependency graph should pass validation.");
    Scene scene;StructuralCommandBuffer commands;ServiceRegistry services;EventQueue events;
    RenderSubmissionQueue render;DeterministicRandom random(42);Profiler profiler;DeterministicJobQueue jobs;
    services.Provide(events,std::string(EventServiceId));
    SystemContext context{.deltaTime=1.0f/60.0f,.tick=1,.components=&scene,.commands=&commands,
                          .services=&services,.events=&events,.render=&render,.random=&random,
                          .profiler=&profiler,.jobs=&jobs};
    Require(systems.Execute(SystemPhase::FixedPrePhysics,context,&error)&&
            systems.Execute(SystemPhase::FixedPhysics,context,&error)&&
            systems.Execute(SystemPhase::FixedBehavior,context,&error),"Registered phases should execute.");
    Require(order==std::vector<std::string>{"prepare","job1","job2","physics","behavior"}&&
            commands.Pending().size()==1&&events.Pending().size()==1,
            "Systems should receive deterministic jobs, commands, and events through context services.");
    ProjectSystemDescriptor needsService{"needs-service","sample",SystemPhase::VariableUpdate,{}, {},{},false,false,false,[](auto&) {}};
    needsService.requiredServices={std::string(SpatialServiceId)};
    SystemRegistry serviceChecked;Require(serviceChecked.Register(std::move(needsService))&&
        !serviceChecked.Execute(SystemPhase::VariableUpdate,context,&error)&&error.find("pipeframe.spatial-2d")!=std::string::npos,
        "A system must fail before execution when a declared host service is unavailable.");

    SystemRegistry invalid;
    Require(invalid.Register({"a","sample",SystemPhase::FixedBehavior,{"b"},{},{"pose"},true,false,false,[](auto&){} })&&
            invalid.Register({"b","sample",SystemPhase::FixedBehavior,{"a"},{},{"pose"},true,false,false,[](auto&){} })&&
            !invalid.Validate().empty(),"Cycles and parallel writer conflicts should fail validation.");
    Require(invalid.Register({"render-mutation","sample",SystemPhase::Render,{}, {},{},false,false,true,[](auto&){} })&&
            !invalid.Validate().empty(),"Render-time structural mutation should fail lifecycle validation.");
    SystemRegistry writers;
    Require(writers.Register({"writer-a","sample",SystemPhase::FixedBehavior,{}, {},{"pose"},false,false,false,[](auto&){} })&&
            writers.Register({"writer-b","sample",SystemPhase::FixedBehavior,{}, {},{"pose"},false,false,false,[](auto&){} })&&
            !writers.Validate().empty(),"Conflicting writers should require an explicit dependency.");
    SystemRegistry isolated;
    Require(isolated.Register({"throwing","sample",SystemPhase::FixedBehavior,{}, {},{},false,false,false,[](auto&){throw std::runtime_error("failure");}})&&
            !isolated.Execute(SystemPhase::FixedBehavior,context,&error)&&error.find("throwing")!=std::string::npos,
            "A project system failure should be isolated and attributed to its stable ID.");
    std::cout<<"All plugin API tests passed.\n";
}
