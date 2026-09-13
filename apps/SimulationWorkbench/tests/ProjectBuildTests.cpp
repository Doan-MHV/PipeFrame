#include <PipeFrame/Components/KinematicBody2DComponent.h>
#include "../Editor/ProjectBuildPlan.h"
#include "../Editor/ProjectSession.h"
#include "../Editor/ProjectScaffolder.h"
#include <PipeFrame/Components/PlaygroundComponent.h>
#include <PipeFrame/Components/TilemapComponent.h>
#include <PipeFrame/Environment/TilemapSerializer.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
using namespace pipeframe;
using namespace pipeframe::editor;
void Check(bool value,const std::string &error){if(!value)throw std::runtime_error(error);}
ProcessProgress Build(ProcessTask &task,const ProjectBuildPlan &plan){
    std::string error;Check(task.Start(plan.commands,plan.project,plan.project/".pipeframe/build.log",error),error);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(120);
    for(;;){auto progress=task.Poll();if(!progress.running)return progress;
        Check(std::chrono::steady_clock::now()<deadline,"Build timeout");std::this_thread::sleep_for(std::chrono::milliseconds(20));}
}
int main(){try{
    const auto root=std::filesystem::temp_directory_path()/("pipeframe R6 build "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ProjectSession project(root/"recent");std::string error;
    Check(project.CreateProjectAt(root/"Soldier Project",&error),error);
    const auto directory=project.GetProjectDirectory();
    Check(ProjectScaffolder::AddModule(directory,GeneratedModuleKind::Entity,"SoldierAnt",&error),error);
    Check(ProjectScaffolder::AddModule(directory,GeneratedModuleKind::Component,"SoldierSettings",&error),error);
    Check(ProjectScaffolder::AddModule(directory,GeneratedModuleKind::Behaviour,"SoldierBehaviour",&error),error);
    Check(ProjectScaffolder::AddModule(directory,GeneratedModuleKind::Brush,"TemperatureBrush",&error),error);
    const std::string settings=R"(#pragma once
#include <PipeFrame/Project/ComponentSchema.h>
struct SoldierSettings {
 float speed=2;
 static auto Schema(){return pipeframe::ComponentSchema<SoldierSettings>("project.SoldierSettings","Soldier Settings")
 .Editable({.key="speed",.displayName="Speed",.kind=pipeframe::PropertyKind::Number,.defaultValue=2.0,.minimum=0.0,.maximum=10.0},&SoldierSettings::speed);}
};
)";
    {std::ofstream out(directory/"Source/Components/SoldierSettings.h");out<<settings;}
    {std::ofstream out(directory/"Source/Behaviours/SoldierBehaviour.h");out<<R"(#pragma once
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Components/Transform2DComponent.h>
#include "Components/SoldierSettings.h"
class SoldierBehaviour final:public pipeframe::Behaviour {
 void FixedUpdate(float dt) override {
   auto *settings=GetComponent<SoldierSettings>();auto *transform=GetComponent<pipeframe::Transform2DComponent>();
   if(settings&&transform)transform->position.x+=settings->speed*dt;
 }
};
)";}
    const auto plan=ProjectBuildPlan::Create(directory,PIPEFRAME_SDK_DIR,PIPEFRAME_HOST_BUILD_DIR,
        PIPEFRAME_HOST_CONFIGURATION,PIPEFRAME_CMAKE_COMMAND,PIPEFRAME_ENGINE_LIBRARY);
    ProcessTask task;auto result=Build(task,plan);Check(result.exitCode==0,result.output);
    Check(project.LoadBuiltRuntime(plan.library,&error),error);
    Check(project.GetManifest()->runtimeLibrary==plan.library,"Build must persist runtime manifest path");
    Check(project.CreateObjectOfType(PlaygroundEntityTypeId,Vector2f{-100,-80}),"Built-in Playground must be available without project-specific registration");
    const auto playgroundId=*project.GetSelectedObjectId();
    Check(project.SetSelectedComponentProperty(PlaygroundComponentTypeId,"columns",std::int64_t{16}),"Edit Playground dimensions through its schema");
    Check(!project.SetSelectedComponentProperty(PlaygroundComponentTypeId,"cellSize",0.0),"Reject invalid cell size");
    Check(project.Undo(),"Undo Playground dimension edit");
    Check(project.Redo(),"Redo Playground dimension edit");
    Tilemap2D painted(16,24,10);painted.DefineTile({1,{200,80,30,255},true});painted.AddLayer("Terrain");painted.SetTile(0,{3,4},1);
    const auto tilePath=directory/"painted.pftilemap";
    {std::ofstream out(tilePath);Check(TilemapSerializer::Save(painted,out),"Save authored tile fixture");}
    const auto tileAsset=project.GetAssetDatabase().ImportNow({tilePath},&error);Check(tileAsset.has_value(),error);
    Check(project.AssignAssetToSelectedProperty(TilemapComponentTypeId,"asset",*tileAsset),"Assign tilemap to the standard component field");
    Check(project.BeginTilemapEditing(*tileAsset),"Open authored map");
    auto &brushEditor=project.GetTilemapEditor();
    Check(brushEditor.Brushes().size()==1&&brushEditor.SelectBrush("project.TemperatureBrush"),"Generated brush compiled and discovered without handwritten registration");
    Check(brushEditor.SetBrushSetting("amount",.75),"Generated brush settings work");
    Check(project.Save(&error)&&!brushEditor.IsDirty()&&!project.GetDocument().IsDirty(),"Save before testing capture-only switch protection");
    const InputEvent brushDown{InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}};
    brushEditor.HandleEvent(brushDown,GridCoordinate{2,2},true);
    Check(brushEditor.HasPointerCapture(),"Capture generated brush before reload");
    Check(!project.OpenProject(directory,&error)&&brushEditor.HasPointerCapture(),"Project switching cannot discard an active brush gesture");
    Check(project.LoadBuiltRuntime(plan.library,&error),error);
    Check(!brushEditor.HasPointerCapture()&&!brushEditor.ActiveBrush()&&brushEditor.Brushes().size()==1,"Reload cancels stroke and replaces plugin-owned catalog");
    Check(brushEditor.Document()->DataLayer("project.TemperatureBrush")->cells.At({2,2})==0,"Reload does not commit pending preview");
    Check(brushEditor.Save()&&brushEditor.Close(),"Close generated brush document");
    Check(project.OpenProject(directory,&error)&&!brushEditor.ActiveBrush()&&!brushEditor.Document(),"Project activation tears down old brush/document state after save/cancel");
    const auto shaderPath=directory/"wrong.glsl";{std::ofstream out(shaderPath);out<<"void main() {}";}
    const auto wrongAsset=project.GetAssetDatabase().ImportNow({shaderPath},&error);Check(wrongAsset.has_value(),error);
    Check(!project.AssignAssetToSelectedProperty(TilemapComponentTypeId,"asset",*wrongAsset),"Typed tilemap slot rejects a shader");
    Check(project.CreateObjectOfType("project.SoldierAnt",Vector2f{0,0}),"Generated entity missing from creation registry");
    const auto id=*project.GetSelectedObjectId();
    Check(project.AddSelectedComponent({.typeId="project.SoldierSettings",.properties={{"speed",2.0}}}),"Attach generated component");
    Check(project.AddSelectedComponent({.typeId="project.SoldierBehaviour"}),"Attach generated behaviour");
    Check(project.SetSelectedComponentProperty("project.SoldierSettings","speed",4.0),"Edit schema field");
    Check(!project.SetSelectedComponentProperty("project.SoldierSettings","speed",-1.0),"Reject invalid schema field");
    project.GetRuntime().BeginPreview();project.GetRuntime().FixedUpdate(.25f);
    auto components=project.GetRuntime().InspectObjectComponents(id);Check(components.has_value(),"Inspect runtime entity");
    bool moved=false;for(const auto &component:*components)if(component.typeId==Transform2DComponentTypeId)
        moved=std::get<Vector2f>(component.properties.at("position")).x==1.0f;
    Check(moved,"Generated behaviour must consume edited speed and update transform");project.GetRuntime().EndPreview();
    Check(project.CreateObjectOfType("project.SoldierAnt",Vector2f{-90,-35}),"Create body entity through generated picker registry");
    const auto bodyId=*project.GetSelectedObjectId();
    Check(project.AddSelectedComponent({.typeId=KinematicBody2DComponentTypeId}),"Attach engine kinematic component without project registration");
    Check(project.SetSelectedComponentProperty(KinematicBody2DComponentTypeId,"velocity",Vector2f{1000,0}),"Edit body velocity through schema");
    Check(project.SetSelectedComponentProperty(KinematicBody2DComponentTypeId,"radius",1.0),"Edit body radius through schema");
    Check(!project.SetSelectedComponentProperty(KinematicBody2DComponentTypeId,"radius",-1.0),"Reject negative body radius");
    const auto stoppedAtWall=[&](ProjectSession &session){
        session.GetRuntime().BeginPreview();session.GetRuntime().FixedUpdate(.1f);
        const auto body=session.GetRuntime().InspectObjectComponents(bodyId);bool stopped=false;
        if(body)for(const auto &component:*body)if(component.typeId==Transform2DComponentTypeId)
            stopped=std::abs(std::get<Vector2f>(component.properties.at("position")).x+71.f)<.001f;
        session.GetRuntime().EndPreview();return stopped;
    };
    Check(stoppedAtWall(project),"Generated plugin body stops at assigned authored tilemap");
    project.SetSelectedObject(id);
    Check(project.CreatePrefabFromSelected("SoldierPrefab",&error),error);
    Check(project.Save(&error),error);
    const auto count=project.GetDocument().GetObjects().size();
    {std::ofstream out(directory/"Source/Components/SoldierSettings.h",std::ios::app);out<<"\nTHIS_IS_A_COMPILER_ERROR\n";}
    result=Build(task,plan);Check(result.exitCode!=0,"Bad source must fail build");
    Check(project.GetRuntime().HasRuntime()&&project.GetDocument().GetObjects().size()==count,"Build failure must preserve runtime and scene");
    {std::ofstream out(directory/"Source/Components/SoldierSettings.h");out<<settings;}
    result=Build(task,plan);Check(result.exitCode==0,result.output);
    Check(project.LoadBuiltRuntime(plan.library,&error),error);
    {std::ofstream out(directory/"broken.dylib");out<<"not a library";}
    Check(!project.LoadBuiltRuntime("broken.dylib",&error),"Invalid runtime must fail activation");
    Check(project.GetRuntime().HasRuntime()&&project.GetManifest()->runtimeLibrary==plan.library,"Failed reload must preserve runtime and manifest");
    ProjectSession reopened(root/"recent2");Check(reopened.OpenProject(directory,&error),error);
    Check(reopened.GetRuntime().HasRuntime(),"Reopen must load generated runtime from manifest");
    Check(reopened.GetDocument().GetObjects().size()==count,"Reopen must preserve scene");
    const auto groundComponents=reopened.GetRuntime().InspectObjectComponents(playgroundId);
    Check(groundComponents.has_value(),"Reopen must restore Playground ECS components");
    const auto ground=std::ranges::find(*groundComponents,std::string(PlaygroundComponentTypeId),&SceneComponentData::typeId);
    Check(ground!=groundComponents->end() && std::get<std::int64_t>(ground->properties.at("columns"))==16,
          "Playground settings must survive generated build/reload and scene save/reopen");
    const auto tileComponent=std::ranges::find(*groundComponents,std::string(TilemapComponentTypeId),&SceneComponentData::typeId);
    Check(tileComponent!=groundComponents->end() && std::get<AssetReference>(tileComponent->properties.at("asset")).assetId==*tileAsset,
          "Tilemap stable asset reference survives reopen");
    Check(stoppedAtWall(reopened),"Saved/reopened generated project retains engine movement collision");
    Check(reopened.InstantiatePrefab("SoldierPrefab",0,{},&error),error);
    std::cout<<"Generated CMake build, entity picker registry, attachment, validation, behaviour execution, prefab/reopen, compiler failure and reload recovery passed\n";
    project.GetRuntime().Unload();reopened.GetRuntime().Unload();std::filesystem::remove_all(root);
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
