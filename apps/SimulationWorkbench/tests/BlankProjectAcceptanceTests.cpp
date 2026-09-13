#include "../Editor/ProjectBuildPlan.h"
#include "../Editor/ProjectSession.h"
#include "../Editor/ProjectScaffolder.h"
#include "../Runtime/SimulationSession.h"
#include <PipeFrame/Components/KinematicBody2DComponent.h>
#include <PipeFrame/Components/PlaygroundComponent.h>
#include <PipeFrame/Components/TilemapComponent.h>
#include <PipeFrame/Components/EnvironmentCollider2DComponent.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
using namespace pipeframe;
using namespace pipeframe::editor;
namespace {
void Check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
ProcessProgress Build(ProcessTask &task,const ProjectBuildPlan &plan){
    std::string error;Check(task.Start(plan.commands,plan.project,plan.project/".pipeframe/build.log",error),error);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(120);
    for(;;){auto progress=task.Poll();if(!progress.running)return progress;
        Check(std::chrono::steady_clock::now()<deadline,"Build timeout");std::this_thread::sleep_for(std::chrono::milliseconds(20));}
}
PropertyValue Live(ProjectSession &project,SceneObjectId id,const std::string &type,const std::string &key){
    const auto components=project.GetRuntime().InspectObjectComponents(id);Check(bool(components),"Missing live entity");
    for(const auto &component:*components)if(component.typeId==type)return component.properties.at(key);
    throw std::runtime_error("Missing live component: "+type);
}
}
int main(int argc,char **argv){try{
    const auto root=argc>1?std::filesystem::absolute(argv[1]):std::filesystem::temp_directory_path()/("pipeframe-blank-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Check(!std::filesystem::exists(root),"Use a new output directory; existing work is never overwritten");
    ProjectSession project(root.parent_path()/"recent-blank");std::string error;
    Check(project.CreateProjectAt(root,&error),error);
    for(const auto &[kind,name]:std::vector<std::pair<GeneratedModuleKind,std::string>>{{GeneratedModuleKind::Entity,"Probe"},{GeneratedModuleKind::Component,"ProbeSettings"},{GeneratedModuleKind::Behaviour,"ProbeBehaviour"}})
        Check(ProjectScaffolder::AddModule(root,kind,name,&error),error);
    // These are the exact public-library tutorial files, not embedded test-only logic.
    for(const auto &[folder,name]:std::vector<std::pair<std::string,std::string>>{{"Components","ProbeSettings.h"},{"Behaviours","ProbeBehaviour.h"}})
        std::filesystem::copy_file(std::filesystem::path(PIPEFRAME_SDK_DIR)/"docs/tutorials/blank-project"/name,root/"Source"/folder/name,std::filesystem::copy_options::overwrite_existing);
    const auto plan=ProjectBuildPlan::Create(root,PIPEFRAME_SDK_DIR,PIPEFRAME_HOST_BUILD_DIR,PIPEFRAME_HOST_CONFIGURATION,PIPEFRAME_CMAKE_COMMAND,PIPEFRAME_ENGINE_LIBRARY);
    ProcessTask task;auto result=Build(task,plan);Check(result.exitCode==0,result.output);Check(project.LoadBuiltRuntime(plan.library,&error),error);
    Check(ProjectScaffolder::Validate(root).empty(),"Generated project follows the standard structure");
    Check(project.CreateObjectOfType(PlaygroundEntityTypeId,Vector2f{}),"Create Playground");const auto ground=*project.GetSelectedObjectId();
    Check(project.CreateTilemapAsset(),"New Map creates and assigns an empty map through the editor");
    auto &map=project.GetTilemapEditor();const auto asset=map.AssetId();
    Check(map.Document()->Tile(0,{12,10})==0,"New map starts empty");
    const auto stroke=[&](GridCoordinate from,GridCoordinate to){
        map.ConfigureGesture(0,1,TilemapPaintShape::Line);
        Check(map.HandleEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}},from,true),"Begin map gesture");
        Check(map.HandleEvent({InputEventType::PointerMoved,PointerMoveInput{{}}},to,true),"Preview map gesture");
        Check(map.HandleEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,{}}},to,true),"Commit map gesture");
    };
    // Coordinates are authored gestures, never baked into the behaviour or engine.
    stroke({12,0},{12,18});stroke({22,5},{22,23});stroke({3,18},{12,18});
    Check(map.Undo()&&map.Document()->Tile(0,{3,18})==0,"Undo the whole last stroke");
    Check(map.Redo()&&map.Document()->Tile(0,{3,18})==1,"Redo exact stroke");
    Check(map.Save()&&map.Close(),"Persist editor-authored maze");
    auto &visual=project.GetVisualAssetEditor();
    Check(visual.Create(project.GetAssetDatabase(),assets::AssetType::Material),visual.LastError());
    Check(visual.Set("tint",Color{55,70,85,255})&&visual.Save(),visual.LastError());const auto material=visual.AssetId();
    Check(visual.Close(),visual.LastError());
    project.SetSelectedObject(ground);Check(project.AssignAssetToSelectedProperty(PlaygroundComponentTypeId,"material",material),"Assign floor material");
    const auto marker=[&](const char *name,Vector2f position,Color color,bool collision){
        Check(project.CreateObjectOfType(EnvironmentObstacleEntityTypeId,position),"Create marker/obstacle from shared entity");
        Check(project.RenameSelectedObject(name),"Name scene role");
        Check(project.SetSelectedComponentProperty(EnvironmentCollider2DComponentTypeId,"color",color),"Set marker appearance");
        Check(project.SetSelectedComponentProperty(EnvironmentCollider2DComponentTypeId,"size",Vector2f{10,10}),"Set marker size");
        if(!collision)Check(project.SetSelectedComponentProperty(EnvironmentCollider2DComponentTypeId,"enabled",false),"Disable marker collision");
        return *project.GetSelectedObjectId();
    };
    marker("Spawn",{30,105},{60,210,100,255},false);
    marker("Goal",{280,105},{240,180,40,255},false);
    marker("Obstacle",{60,120},{180,95,70,255},true);
    Check(project.CreateObjectOfType("project.Probe",Vector2f{30,105}),"Create generated Probe");const auto probe=*project.GetSelectedObjectId();
    Check(project.AddSelectedComponent({.typeId="project.ProbeSettings"}),"Attach generated settings");
    Check(project.AddSelectedComponent({.typeId=KinematicBody2DComponentTypeId}),"Attach shared movement");
    Check(project.SetSelectedComponentProperty(KinematicBody2DComponentTypeId,"radius",2.0),"Edit radius");
    Check(project.AddSelectedComponent({.typeId="project.ProbeBehaviour"}),"Attach generated behaviour");
    Check(project.AddSelectedComponent({.typeId=EnvironmentCollider2DComponentTypeId}),"Attach visible marker");
    Check(project.SetSelectedComponentProperty(EnvironmentCollider2DComponentTypeId,"enabled",false),"Marker must not hit its own ray");
    Check(project.SetSelectedComponentProperty(EnvironmentCollider2DComponentTypeId,"size",Vector2f{6,6}),"Size marker");
    Check(!project.SetSelectedComponentProperty("project.ProbeSettings","stopDistance",200.0),"Component-owned validation rejects invalid authored data");
    Check(!project.SetSelectedComponentProperty("project.ProbeSettings","nearest",10.0),"Read-only telemetry rejects editor writes");
    Check(project.CreatePrefabFromSelected("ProbePrefab",&error),error);Check(project.Save(&error),error);
    const auto run=[&](ProjectSession &session){
        SimulationSession simulation(session.GetRuntime());simulation.Toggle(session.GetDocument().GetObjects());
        for(int i=0;i<300;++i)simulation.FixedUpdate(1.f/60.f);
        const auto pose=std::get<Vector2f>(Live(session,probe,Transform2DComponentTypeId,"position"));
        Check(pose.x>100&&pose.x<120,"Probe advances and stops before painted wall");
        Check(std::get<bool>(Live(session,probe,"project.ProbeSettings","blocked")),"Behaviour obtains scene query service and publishes blocked telemetry");
        simulation.Toggle(session.GetDocument().GetObjects());simulation.FixedUpdate(1.f);
        Check(std::get<Vector2f>(Live(session,probe,Transform2DComponentTypeId,"position"))==pose,"Pause freezes runtime movement");
        simulation.Reset(session.GetDocument().GetObjects());
        Check(std::get<Vector2f>(Live(session,probe,Transform2DComponentTypeId,"position"))==Vector2f{30,105},"Reset restores authored spawn position");
        Check(std::get<double>(Live(session,probe,"project.ProbeSettings","nearest"))==-1,"Reset restores telemetry defaults");
        simulation.Stop();
    };
    run(project);Check(project.Save(&error),error);
    ProjectSession reopened(root.parent_path()/"recent-blank-reopen");Check(reopened.OpenProject(root,&error),error);
    Check(reopened.GetDocument().GetObjects().size()==5,"Reopen retains playground, obstacle, spawn, goal and probe");
    reopened.SetSelectedObject(ground);Check(reopened.BeginTilemapEditing(asset),"Reopen map using stable asset ID");
    Check(reopened.GetTilemapEditor().Document()->Tile(0,{12,10})==1,"Painted maze persists");Check(reopened.GetTilemapEditor().Close(),"Close reopened map");run(reopened);
    Check(reopened.InstantiatePrefab("ProbePrefab",0,{30,120},&error),error);const auto clone=*reopened.GetSelectedObjectId();
    reopened.GetRuntime().BeginPreview();for(int i=0;i<120;++i)reopened.GetRuntime().FixedUpdate(1.f/60.f);
    Check(std::get<bool>(Live(reopened,clone,"project.ProbeSettings","blocked")),"Instantiated prefab queries the independent box obstacle");reopened.GetRuntime().EndPreview();
    Check(reopened.Save(&error),error);
    result=Build(task,plan);Check(result.exitCode==0,result.output);Check(reopened.LoadBuiltRuntime(plan.library,&error),error);run(reopened);
    Check(reopened.GetDocument().GetObjects().size()==6,"Reload preserves all authored entities and prefab instance");
    project.GetRuntime().Unload();reopened.GetRuntime().Unload();
    std::cout<<"PASS: blank project -> generate/build -> Playground/material/New Map -> gestures/undo/redo -> spawn/goal/obstacle -> settings/behaviour/body -> run/pause/reset -> prefab/save/reopen/rebuild/reload\n";
    if(argc==1)std::filesystem::remove_all(root);else std::cout<<"Retained tutorial project: "<<root<<'\n';
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
