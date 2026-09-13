#include "Runtime/AntSimulationRuntime.h"
#include "Runtime/AntRegistration.h"
#include "Editor/AntFoodBrush.h"
#include <PipeFrame/Editor/TilemapAssetEditor.h>
#include "SceneSerializer.h"
#include <fstream>
#include <iostream>
using namespace pipeframe;
using namespace ant_simulation;
static void Check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
static void Field(SceneObjectData &object,const char *type,const char *key,PropertyValue value){
    for(auto &component:object.components)if(component.typeId==type){component.properties[key]=std::move(value);return;}
    throw std::runtime_error("Missing component");
}
int main(){try{
    const auto root=std::filesystem::temp_directory_path()/"pipeframe-ant-authoring-acceptance";
    std::filesystem::remove_all(root);std::filesystem::create_directories(root/"Assets/Tilemaps");
    assets::AssetDatabase database;std::string error;Check(database.Open(root,&error),"Open fresh project assets");
    Tilemap2D map(32,24);map.AddLayer("Terrain");map.DefineTile({1,{45,42,38,255},true});
    const auto path=root/"Assets/Tilemaps/Maze.pftilemap";
    {std::ofstream out(path);Check(TilemapSerializer::Save(map,out),"Create empty editor-authored map");}
    const auto asset=database.ImportNow({path},&error);Check(bool(asset),"Import authored environment");
    Check(database.Find("source:Assets/Tilemaps/Maze.pftilemap")->id==*asset,"Portable source scene resolves imported asset");
    ServiceRegistry services;services.Provide(database);ProjectRuntimeContext context;context.services=&services;
    AntSimulationRuntime runtime;Check(runtime.Load(context,error),"Load Ant");
    auto ground=runtime.CreateDefaultObject(PlaygroundEntityTypeId);ground.id=1;
    Field(ground,TilemapComponentTypeId,"asset",AssetReference{*asset});
    auto colony=runtime.CreateDefaultObject(ColonyTypeId);colony.id=2;colony.transform.position={8,8};SynchronizeTransformComponent(colony);
    Field(colony,ColonyTypeId,InitialPopulationKey,std::int64_t{10});
    auto food=runtime.CreateDefaultObject(FoodSourceTypeId);food.id=3;food.transform.position={24,8};SynchronizeTransformComponent(food);
    Field(food,FoodSourceTypeId,FoodRadiusKey,1.0);
    std::vector<SceneObjectData> objects{ground,colony,food};runtime.SynchronizeScene(objects);
    Check(runtime.GetSimulationWorld()->GetEnvironment().GetWidth()==32,"Playground owns environment dimensions");
    TilemapAssetEditor editor;Check(editor.Open(database,*asset),"Open map in shared editor");editor.SetEnabled(true);
    const auto gesture=[&](GridCoordinate a,GridCoordinate b){
        editor.HandleEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}},a,true);
        editor.HandleEvent({InputEventType::PointerMoved,PointerMoveInput{}},b);
        editor.HandleEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,{}}},b);
    };
    editor.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Rectangle);gesture({14,5},{15,18});
    Check(editor.Save(),"Save wall rectangle");runtime.Start();for(int i=0;i<10;++i)runtime.FixedUpdate(1.f/60);
    Check(runtime.GetSimulationWorld()->GetEnvironment().TryGetCell(14,10)->wall,"Saved painting creates simulation wall");
    Check(runtime.GetSimulationWorld()->GetAntQuery().GetCount()==10,"Authored colony spawns after map editing");
    auto brush=std::make_shared<AntFoodBrush>();brush->quantity=23;
    // The host normally instantiates this through the registered brush factory.
    Check(editor.AddDataLayer("ant.food-density",0,10000),"Add Ant data target");
    editor.Configure(0,brush,TilemapPaintShape::Line);gesture({20,14},{23,14});Check(editor.Save(),"Save Ant brush data");
    runtime.Reset();Check(runtime.GetSimulationWorld()->GetEnvironment().TryGetCell(21,14)->foodQuantity==23,"Authored food density enters runtime exactly");
    Check(editor.Undo()&&editor.Save(),"Undo food stroke and save");runtime.Reset();
    Check(runtime.GetSimulationWorld()->GetEnvironment().TryGetCell(21,14)->foodQuantity==0,"Undo removes authored food");
    Check(editor.Redo()&&editor.Save(),"Redo food stroke and save");runtime.Reset();
    pipeframe::editor::SceneDocument scene;for(auto object:objects)Check(scene.RestoreObject(std::move(object)),"Persist scene object");
    Check(pipeframe::editor::SceneSerializer::Save(scene,root/"Main.pfscene",&error),"Save authored Ant scene");
    auto reopened=pipeframe::editor::SceneSerializer::Load(root/"Main.pfscene",&error);Check(bool(reopened),"Reopen scene through editor serializer");
    AntSimulationRuntime reloaded;Check(reloaded.Load(context,error),"Reload Ant runtime");reloaded.SynchronizeScene(reopened->GetObjects());
    Check(reloaded.GetSimulationWorld()->GetEnvironment().TryGetCell(14,10)->wall&&reloaded.GetSimulationWorld()->GetEnvironment().TryGetCell(21,14)->foodQuantity==23,"Reopen preserves wall and food authoring");
    reloaded.Start();for(int i=0;i<10;++i)reloaded.FixedUpdate(1.f/60);reloaded.Reset();
    Check(reloaded.GetSimulationWorld()->GetEnvironment().TryGetCell(21,14)->foodQuantity==23,"Reset restores authored food after running");
    auto invalid=objects;invalid.front().transform.scale={2,2};SynchronizeTransformComponent(invalid.front());
    bool rejected=false;try{reloaded.SynchronizeScene(invalid);}catch(const std::invalid_argument &){rejected=true;}
    Check(rejected&&reloaded.GetSimulationWorld()->GetEnvironment().GetWidth()==32,"Unsupported scale rejects without losing prior world");
    reloaded.Start();auto *priorWorld=reloaded.GetSimulationWorld();
    Tilemap2D unsupported(32,24,2);unsupported.AddLayer("Terrain");
    {std::ofstream out(path);TilemapSerializer::Save(unsupported,out);}
    Check(database.Reimport(*asset,&error),"Reimport unsupported Ant cell scale");reloaded.FixedUpdate(1.f/60);
    Check(reloaded.GetSimulationWorld()==priorWorld&&priorWorld->GetEnvironment().TryGetCell(14,10)->wall,"Invalid saved revision retains the existing simulation");
    {std::ofstream out(path);TilemapSerializer::Save(*editor.Document(),out);}
    Check(database.Reimport(*asset,&error),"Restore valid authored revision");reloaded.FixedUpdate(1.f/60);
    Check(reloaded.GetSimulationWorld()->GetStatistics().tick==1&&reloaded.GetSimulationWorld()->GetEnvironment().TryGetCell(21,14)->foodQuantity==23,"Valid revision recovers at tick boundary and preserves playing mode");
    Check(editor.Close(true),"Close authoring document");reloaded.Unload();runtime.Unload();
    std::cout<<"Ant Playground: create, paint, food brush, run, undo/redo, scene save/reopen, reload, reset and invalid-transform rollback passed.\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
