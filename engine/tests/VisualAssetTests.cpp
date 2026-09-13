#include <PipeFrame/Editor/VisualAssetEditor.h>
#include <PipeFrame/Editor/TilemapAssetEditor.h>
#include <PipeFrame/Project/SceneProjectRuntime.h>
#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include <SFML/Graphics/Image.hpp>
#include <future>
#include <thread>
#include <iostream>
using namespace pipeframe;
using namespace pipeframe::assets;
void Check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void Drain(AssetDatabase &db){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    do {db.PumpOperations();if(!db.HasActiveImport())return;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<deadline);
    throw std::runtime_error("Import timeout");
}
int main(){try{
    const auto root=std::filesystem::temp_directory_path()/"pipeframe-visual-asset-acceptance";
    std::filesystem::remove_all(root);std::filesystem::create_directories(root);
    AssetDatabase db;std::string error;Check(db.Open(root,&error),"Open blank asset project");
    ImageData pixels({8,4},{255,0,0,255});for(unsigned y=0;y<4;++y)for(unsigned x=4;x<8;++x)pixels.SetPixel({x,y},{0,255,0,255});
    Check(SaveImageData(root/"atlas.png",pixels,error),"Write two-color atlas");
    const auto texture=db.ImportNow({root/"atlas.png"},&error);Check(bool(texture),"Import actual texture");
    VisualAssetEditor editor;Check(editor.Create(db,AssetType::Material),"Create material without hand-written registration");const auto material=editor.AssetId();
    Check(editor.Set("texture",AssetReference{*texture})&&editor.Set("uvScale",Vector2f{2,1}),"Material texture and UV repeats");
    Check(!editor.Set("uvScale",Vector2f{0,1}),"Reject invalid repeat scale");
    Check(editor.Save(),"Save material");Check(db.Find(material)->dependencies==std::vector<std::string>{*texture},"Material dependency extracted");
    Check(editor.Set("tint",Color{128,255,255,255})&&editor.Undo()&&!editor.IsDirty()&&editor.Redo()&&editor.IsDirty(),"Material history and savepoint");
    Check(editor.Undo()&&editor.Close()&&editor.Create(db,AssetType::Tileset),"Switch clean asset documents");const auto tileset=editor.AssetId();
    Check(!editor.Set("material",AssetReference{*texture}),"Reject wrong reference type");
    Check(editor.Set("material",AssetReference{material})&&!editor.Save(),"Reject atlas larger than image");
    Check(editor.Set("tileWidth",std::int64_t{4})&&editor.Set("tileHeight",std::int64_t{4})&&editor.Set("columns",std::int64_t{2})&&editor.Save(),"Save validated atlas");
    Check(db.Find(tileset)->dependencies==std::vector<std::string>{material},"Atlas dependency extracted");
    Tilemap2D map(4,2,16);map.AddLayer("Ground");
    {std::ofstream output(root/"map.pftilemap");Check(TilemapSerializer::Save(map,output),"Create blank map");}
    const auto mapId=db.ImportNow({root/"map.pftilemap"},&error);Check(bool(mapId),"Import map");
    TilemapAssetEditor painter;Check(painter.Open(db,*mapId)&&painter.AssignTileset(tileset),"Assign atlas to map");
    painter.SetEnabled(true);painter.Configure(0,std::make_shared<TileBrush>(2));
    Check(painter.HandleEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}},GridCoordinate{3,1},true),"Begin paint");
    painter.HandleEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,{}}},GridCoordinate{3,1});
    Check(painter.Document()->Tile(0,{3,1})==2,"Paint atlas tile");
    Check(painter.PrepareResize(2,2,16)&&painter.ResizeSummary().find("1 painted cells cropped")!=std::string::npos&&painter.Document()->Columns()==4,"Crop preview leaves map intact");
    Check(painter.ApplyResize()&&painter.Document()->Columns()==2&&painter.Undo()&&painter.Document()->Tile(0,{3,1})==2,"Crop undo restores cells");
    Check(!painter.PrepareResize(8,2,16,true),"Reject non-square resampling");
    Check(painter.PrepareResize(8,4,16,true)&&painter.ApplyResize()&&painter.Document()->CellSize()==8&&painter.Document()->Bounds().size==Vector2f{64,32},"Resolution change preserves physical area");
    Check(painter.Undo()&&painter.Save(),"Save original resolution with atlas");
    Check(db.Find(*mapId)->dependencies==std::vector<std::string>{tileset},"Map dependency extracted");
    auto copied=painter.CopyAsset();Check(bool(copied)&&*copied!=*mapId,"Make unique map");
    AssetDatabase reopened;Check(reopened.Open(root,&error),"Reopen complete asset graph");
    Check(reopened.Find(tileset)&&reopened.Find(material)&&reopened.Find(*texture),"All stable IDs persist");
    TilemapAssetEditor reopenedMap;Check(reopenedMap.Open(reopened,*mapId)&&reopenedMap.Document()->Tileset()==tileset&&reopenedMap.Document()->Tile(0,{3,1})==2,"Map cells and atlas persist");
    ServiceRegistry services;services.Provide(db);ProjectRuntimeContext runtimeContext;runtimeContext.services=&services;
    SceneProjectRuntime runtime("Blank visual acceptance");Check(runtime.Load(runtimeContext,error),"Runtime resources");
    auto ground=runtime.CreateDefaultObject(PlaygroundEntityTypeId);ground.id=1;
    for(auto &component:ground.components){
        if(component.typeId==PlaygroundComponent::Schema().Describe().typeId){component.properties["columns"]=std::int64_t{4};component.properties["rows"]=std::int64_t{2};component.properties["cellSize"]=16.0;component.properties["showGrid"]=false;component.properties["color"]=Color{255,255,255,255};component.properties["material"]=AssetReference{material};}
        if(component.typeId==TilemapComponentTypeId)component.properties["asset"]=AssetReference{*mapId};
    }
    runtime.SynchronizeScene(std::vector<SceneObjectData>{ground});
    sf::RenderTexture target({64,32});auto render=backend::sfml::MakeRenderContext(target);
    render.GetCamera().SetCenter({32,16});render.GetCamera().SetSize({64,32});render.BeginWorld();
    target.clear();runtime.Render(render);target.display();auto image=target.getTexture().copyToImage();
    Check(image.getPixel({4,4})==sf::Color::Red&&image.getPixel({20,4})==sf::Color::Green&&image.getPixel({36,4})==sf::Color::Red,"Ground repeats actual texture twice");
    Check(image.getPixel({52,20})==sf::Color::Green,"Painted tile uses second atlas region rather than full image");
    Check(image.saveToFile((root/"textured-playground.png").string()),"Save native render evidence");
    auto source=root/db.Find(*texture)->sourcePath;Check(SaveImageData(source,ImageData({8,4},{0,0,255,255}),error),"Change source texture");
    const auto revision=db.Find(*texture)->revision;db.QueueReimport(*texture);Check(db.PumpOperations()&&db.HasActiveImport()&&db.Find(*texture)->revision==revision,"Reimport starts without publishing worker data");
    Drain(db);Check(db.Find(*texture)->revision==revision+1,"Publish successful import on main thread");
    target.clear();runtime.Render(render);target.display();Check(target.getTexture().copyToImage().getPixel({52,20})==sf::Color::Blue,"Texture reimport refreshes material and atlas runtime resources");
    const auto goodCache=db.Find(*texture)->cachePath;const auto goodRevision=db.Find(*texture)->revision;
    {std::ofstream out(source);out<<"corrupt image";}
    db.QueueReimport(*texture);db.PumpOperations();Drain(db);
    Check(db.Find(*texture)->revision==goodRevision&&db.Find(*texture)->cachePath==goodCache&&std::filesystem::is_regular_file(root/goodCache),"Failed background reimport retains published cache and revision");
    Check(SaveImageData(source,ImageData({8,4},{0,0,255,255}),error),"Restore valid source");
    // A blocked worker proves pump/cancel never wait for its result or touch published records.
    std::promise<void> release,entered;auto gate=release.get_future().share();auto started=entered.get_future();
    Check(db.RegisterImporter({"test.blocked",1,AssetType::Shader,{".blocked"},[&](const AssetImportContext &context,std::string &error)->std::optional<AssetImportOutput>{entered.set_value();gate.wait();if(context.isCancelled()){error="cancelled";return {};}return {}; }},&error),"Register blocked test importer");
    {std::ofstream out(root/"test.blocked");out<<"input";}
    const auto count=db.GetAssets().size();const auto operation=db.QueueImport({root/"test.blocked"});
    Check(db.PumpOperations(),"Start worker");Check(started.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Worker entered");
    Check(!db.PumpOperations()&&db.GetAssets().size()==count,"UI pump returns while worker blocked");
    Check(!db.Open(root/"other",&error),"Project switch blocked during active import");
    Check(db.CancelOperation(operation),"Cancel running import");release.set_value();Drain(db);
    Check(db.GetAssets().size()==count&&db.GetOperations().back().state==AssetOperationState::Cancelled,"Cancellation publishes no asset");
    std::cout<<"Visual assets, resize, persistence, async cancellation and native textured rendering passed. Evidence: "<<root<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
