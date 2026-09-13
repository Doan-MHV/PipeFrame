#include <PipeFrame/Project/ProjectRuntimeLibrary.h>
#include <PipeFrame/Physics/PhysicsWorld2D.h>
#include <PipeFrame/Resources/GraphicsResourceService.h>
#include <PipeFrame/Render/RenderServices2D.h>
#include <PipeFrame/Spatial/UniformSpatialIndex.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <ranges>

namespace {
void Require(bool condition,const std::string &message){if(!condition){std::cerr<<"FAILED: "<<message<<'\n';std::exit(1);}}
struct Project { const char *name; const char *id; const char *directory; const char *runtime; std::size_t minimumExtensions; std::size_t minimumSystems; };
class Log final:public pipeframe::ProjectLogSink{public:void Write(std::string_view,std::string_view)override{++writes;}std::size_t writes{};};
class Query final:public pipeframe::ComponentQuery{public:std::span<pipeframe::SceneObjectData> Objects()override{return objects;}std::span<const pipeframe::SceneObjectData> Objects()const override{return objects;}std::vector<pipeframe::SceneObjectData> objects;};
bool HasModuleSource(const std::filesystem::path &directory){
    if(!std::filesystem::is_directory(directory))return false;
    for(const auto &entry:std::filesystem::directory_iterator(directory))
        if(entry.is_regular_file()&&entry.path().filename()!="README.md"&&entry.file_size()>0)return true;
    return false;
}
std::string ReadSource(const std::filesystem::path &path){
    std::ifstream input(path);
    return {(std::istreambuf_iterator<char>(input)),{}};
}
}

int main(){
    using namespace pipeframe;
    const std::array projects{
        Project{"Basic","pipeframe.basic-simulation",PIPEFRAME_BASIC_PROJECT,PIPEFRAME_BASIC_RUNTIME,2,2},
        Project{"Ant","pipeframe.ant-simulation",PIPEFRAME_ANT_PROJECT,PIPEFRAME_ANT_RUNTIME,6,3},
#ifndef PIPEFRAME_ANT_REWORK_ONLY
        Project{"SailBoat","pipeframe.sailboat-simulation",PIPEFRAME_SAILBOAT_PROJECT,PIPEFRAME_SAILBOAT_RUNTIME,5,4},
#endif
};
    for(const auto &project:projects){
        std::cerr<<"Checking "<<project.name<<"...\n";
        const std::filesystem::path root=project.directory;
        for(const auto *path:{"Assets","Scenes","Config","Source/Components","Source/Systems","Source/Runtime","Source/Editor","Tests"})
            Require(std::filesystem::is_directory(root/(std::string_view(project.name)=="Ant" && std::string_view(path)=="Source/Systems"?"Source/World/Runtime/Systems":path)),std::string(project.name)+" must expose standard project folder "+path);
        for(const auto *asset:{"Audio","Materials","Models","Parts","Prefabs","Shaders","Textures"})
            Require(std::filesystem::is_directory(root/"Assets"/asset),std::string(project.name)+" must expose Assets/"+asset);
        for(const auto *file:{"Config/ProjectSettings.pipeframe","Config/Input.pipeframe","Config/Physics.pipeframe"})
            Require(std::filesystem::is_regular_file(root/file),std::string(project.name)+" must provide "+file);
        for(const auto *module:{"Source/Components","Source/Systems","Source/Runtime","Source/Editor"})
            Require(HasModuleSource(root/(std::string_view(project.name)=="Ant" && std::string_view(module)=="Source/Systems"?"Source/World/Runtime/Systems":module)),std::string(project.name)+" must contain a real module source in "+module);
        Require(std::filesystem::is_regular_file(root/"Source/Plugin.cpp"),std::string(project.name)+" must use the standard plugin entry point.");
        for(const auto &entry:std::filesystem::recursive_directory_iterator(root/"Source")){
            if(!entry.is_regular_file())continue;
            std::ifstream input(entry.path());const std::string source((std::istreambuf_iterator<char>(input)),{});
            Require(source.find("SimulationWorkbench")==std::string::npos&&source.find("apps/SimulationWorkbench")==std::string::npos,
                    std::string(project.name)+" project code must not depend on Workbench internals: "+entry.path().string());
        }
        if(std::string_view(project.name)=="Ant"){
            Require(!std::filesystem::exists(root/"Source/Components/Ant.h") &&
                    !std::filesystem::exists(root/"Source/Components/Colony.h"),
                    "Ant and Colony aggregate owners must be removed.");
            Require(ReadSource(root/"Source/Entities/AntEntity.h").find("world.Add<AntIdentityComponent>")!=std::string::npos &&
                    ReadSource(root/"Source/Entities/ColonyEntity.h").find("world.Add<ColonyStateComponent>")!=std::string::npos,
                    "Reference entities must compose scene components.");
            Require(ReadSource(root/"Source/World/Runtime/AntQuery.h").find("pipeframe::SceneViewCache<AntView, AntIdentityComponent>")!=std::string::npos,
                    "Ant queries must use engine borrowed-view indexing.");
            Require(ReadSource(root/"Source/World/Runtime/ColonyHistory.h").find("pipeframe::SampleHistory<ColonyHistorySample>")!=std::string::npos,
                    "Colony history must use the reusable PipeFrame sample history.");
            Require(ReadSource(root/"Source/World/Physics/AntMovementSystem.h").find("pipeframe::FixedUpdateSystem<AntMovementResult>")!=std::string::npos,
                    "Ant movement must be a real PipeFrame fixed-update system.");
            Require(ReadSource(root/"Source/World/Runtime/Systems/AntForagingSystem.h").find("pipeframe::FixedUpdateSystem<void>")!=std::string::npos,
                    "Ant behavior must be a real PipeFrame fixed-update system.");
            Require(ReadSource(root/"Source/World/Runtime/Systems/AntCleanupSystem.h").find("pipeframe::FixedUpdateSystem<AntCleanupResult>")!=std::string::npos,
                    "Ant cleanup must be a real PipeFrame fixed-update system.");
            const auto registration=ReadSource(root/"Source/Runtime/AntRegistration.cpp");
            Require(registration.find("PF_COMPONENT")==std::string::npos && registration.find(".Field(")==std::string::npos,
                    "Ant registration must not define component fields.");
            for(const auto *name:{"ColonySettingsComponent","ColonyStateComponent","ColonyHistoryComponent",
                                 "AntIdentityComponent","AntPoseComponent","ForagingComponent","AntEncounterComponent",
                                 "FoodSourceComponent","SimulationSettingsComponent"}) {
                const auto component=ReadSource(root/"Source/Components"/(std::string(name)+".h"));
                Require(component.find("Schema()")!=std::string::npos &&
                        component.find("ComponentSchema<"+std::string(name)+">")!=std::string::npos &&
                        registration.find(std::string(name)+"::Schema()")!=std::string::npos,
                        "Ant components must own the typed schemas consumed by registration.");
            }
            Require(ReadSource(root/"Source/World/Physics/ContactSolver.h").find("pipeframe::PhysicsSolver<AntBodySystem, ContactSolverResult>")!=std::string::npos,
                    "Ant contact solving must implement the PipeFrame physics-solver contract.");
            Require(ReadSource(root/"Source/Editor/AntEditorTool.h").find("pipeframe::EditorTool")!=std::string::npos,
                    "Ant editor tools must implement the PipeFrame editor-tool contract.");
        }
        if(std::string_view(project.name)=="SailBoat"){
            Require(ReadSource(root/"Source/Systems/BoatMovementSystem.h").find("pipeframe::EntityUpdateSystem<Boat, BoatEnvironment, BoatUpdateCommand>")!=std::string::npos,
                    "BoatMovementSystem must implement the PipeFrame entity-update contract.");
        }
        ProjectRuntimeLibrary library;std::string error;
        Require(library.Load(project.runtime,&error),std::string(project.name)+" runtime should load: "+error);
        auto *runtime=library.GetRuntime();const auto descriptor=runtime->GetPluginDescriptor();
        Require(descriptor.id==project.id&&descriptor.abiVersion==ProjectPluginAbiVersion&&descriptor.usesSystemScheduler,
                std::string(project.name)+" must declare the compatible scheduled plugin contract.");
        ExtensionRegistry extensions;ActionRegistry actions;SystemRegistry systems;
        PluginRegistrar registrar(descriptor.id,extensions,actions,systems);
        Require(runtime->RegisterPlugin(registrar,error),std::string(project.name)+" registration should succeed: "+error);
        Require(extensions.All().size()>=project.minimumExtensions&&systems.All().size()>=project.minimumSystems&&
                    extensions.Validate().empty()&&systems.Validate().empty()&&actions.Conflicts().empty(),
                std::string(project.name)+" extension and system graphs should conform.");
        if(std::string_view(project.name)=="Ant"){
            const auto componentTypes=runtime->GetAllSceneComponentTypes();
            const auto hasComponent=[&](const std::string_view id){return std::ranges::any_of(componentTypes,[&](const auto &item){return item.typeId==id;});};
            Require(hasComponent(Transform2DComponentTypeId)&&hasComponent("pipeframe.physics-body2d")&&
                    hasComponent("ant.colony")&&hasComponent("ant.food-source")&&hasComponent("ant.simulation-settings"),
                    "Ant authoring must expose common transform/physics and domain properties to the editor.");
            for(const auto id:{"ant.colony-lifecycle","ant.movement","ant.behavior","ant.cleanup"})
                Require(std::ranges::any_of(systems.All(),[&](const auto &item){return item.id==id;}),
                        std::string("Ant must register executable system ")+id);
        }
        for(const auto &extension:extensions.All())Require(extension.invoke||extension.inspect,
            std::string(project.name)+" extension must expose executable behavior: "+extension.id);
        for(const auto &system:systems.All())Require(!system.requiredServices.empty(),
            std::string(project.name)+" system must declare its PipeFrame service dependencies: "+system.id);
        // Basic is headless. Ant and SailBoat initialize GPU resources and are
        // exercised through their existing render-context integration suites.
        if(std::string_view(project.name)=="Basic"){
            ServiceRegistry services;EventQueue events;RenderSubmissionQueue render;Log log;
            UniformSpatialIndex<SceneObjectId> spatial;spatial.Initialize({{-1000,-1000},{2000,2000}},32);
            PhysicsWorld2D physics({{-1000,-1000},{2000,2000}});GraphicsResourceService resources;SurfaceRegistry surfaces;
            DeterministicRandom random(7);Profiler profiler;DeterministicJobQueue jobs;
            services.Provide(spatial,std::string(SpatialServiceId));services.Provide(physics,std::string(PhysicsServiceId));
            services.Provide(resources,std::string(ResourceServiceId));services.Provide(surfaces,std::string(RenderServiceId));
            services.Provide(events,std::string(EventServiceId));services.Provide(random,std::string(RandomServiceId));
            services.Provide(jobs,std::string(JobServiceId));services.Provide(log,std::string(LogServiceId));
            services.Provide(profiler,std::string(ProfilingServiceId));
            ProjectRuntimeContext runtimeContext{.projectDirectory=root,.services=&services,.events=&events,.renderSubmissions=&render};
            Require(runtime->Load(runtimeContext,error),"Basic should initialize through the public runtime context: "+error);
            std::vector<SceneObjectData> objects;SceneObjectId nextId=1;
            for(const auto &type:runtime->GetSceneObjectTypes()){
                auto object=runtime->CreateDefaultObject(type.typeId);object.id=nextId++;
                for(auto &[key,value]:object.properties)if(key=="agentCount")value=std::int64_t{8};
                objects.push_back(std::move(object));
            }
            runtime->SynchronizeScene(objects);runtime->Start();
            StructuralCommandBuffer commands;
            Query query;query.objects=objects;
            SystemContext systemContext{.deltaTime=1.0f/60.0f,.tick=1,.components=&query,.commands=&commands,.services=&services,
                                        .events=&events,.render=&render,.random=&random,.profiler=&profiler,.jobs=&jobs,.log=&log};
            for(const auto phase:{SystemPhase::FixedPrePhysics,SystemPhase::FixedPhysics,SystemPhase::FixedPostPhysics,
                                  SystemPhase::FixedBehavior,SystemPhase::FixedCleanup})
                Require(systems.Execute(phase,systemContext,&error),"Basic scheduled phase should execute: "+error);
            runtime->Stop();runtime->Unload();
        }
        extensions.Clear();actions.Clear();systems.Clear();library.Unload();
        std::cerr<<project.name<<" passed.\n";
    }
    std::cout<<"All reference project conformance checks passed.\n";
}
