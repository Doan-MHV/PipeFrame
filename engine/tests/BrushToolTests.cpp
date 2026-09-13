#include <PipeFrame/Editor/TilemapAssetEditor.h>
#include <PipeFrame/Project/ProjectRuntime.h>
#include <iostream>
using namespace pipeframe;
void Check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
class MoistureBrush final:public SchemaBrush<MoistureBrush>{
public:
    double amount{.25};
    static inline int begins{},ends{},cancels{},teardowns{};
    static auto Schema(){return ComponentSchema<MoistureBrush>("test.moisture","Moisture")
        .Editable({.key="amount",.displayName="Moisture",.kind=PropertyKind::Number,.defaultValue=.25,.minimum=0,.maximum=1},&MoistureBrush::amount);}
    std::string_view DataTarget()const override{return "test.moisture";}
    double PaintData(GridCoordinate,double before)const override{return std::min(1.0,before+amount);}
    void OnBegin(GridCoordinate)const noexcept override{++begins;}
    void OnEnd()const noexcept override{++ends;}
    void OnCancel()const noexcept override{++cancels;}
    void OnTeardown()const noexcept override{++teardowns;}
};
int main(){try{
    const auto root=std::filesystem::temp_directory_path()/"pipeframe-custom-brush-tests";
    std::filesystem::remove_all(root);std::filesystem::create_directories(root);
    Tilemap2D map(12,12);map.AddLayer("Visual");map.DefineTile({1,{255,0,0,255},true});
    {std::ofstream out(root/"Map.pftilemap");Check(TilemapSerializer::Save(map,out),"Write fixture");}
    assets::AssetDatabase assets;std::string error;Check(assets.Open(root,&error),"Open project");
    auto id=assets.ImportNow({root/"Map.pftilemap"},&error);Check(bool(id),"Import fixture");
    TilemapAssetEditor editor;Check(editor.Open(assets,*id),"Open map");
    editor.SetBrushes({{"test.moisture","Moisture","Tests",[]{return std::make_shared<MoistureBrush>();}}});
    Check(editor.SelectBrush("test.moisture"),"Discover brush and create independent data layer");
    Check(!editor.SetBrushSetting("amount",2.0)&&std::get<double>(editor.ActiveBrush()->Settings().at("amount"))==.25,"Invalid schema edit preserves settings");
    Check(editor.SetBrushSetting("amount",.5),"Validated settings edit");
    const InputEvent down{InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}},move{InputEventType::PointerMoved,PointerMoveInput{}},up{InputEventType::PointerReleased,PointerInput{PointerButton::Left,{}}},escape{InputEventType::KeyPressed,KeyInput{InputKey::Escape}};
    const auto value=[&](GridCoordinate cell){return editor.Document()->DataLayer("test.moisture")->cells.At(cell);};
    const auto stroke=[&](GridCoordinate a,GridCoordinate b){editor.HandleEvent(down,a,true);editor.HandleEvent(move,b);editor.HandleEvent(up,b);};
    editor.ConfigureGesture(0,1,TilemapPaintShape::Pencil);stroke({2,2},{5,2});
    Check(value({2,2})==.5&&value({5,2})==.5&&editor.Document()->Tile(0,{2,2})==0,"Freehand deduplicates effect and preserves visual data");
    editor.ConfigureGesture(0,0,TilemapPaintShape::Rectangle);stroke({2,2},{5,2});
    Check(value({3,2})==0&&editor.Undo()&&value({3,2})==.5,"Shared eraser clears numeric data with undo");
    Check(editor.Undo()&&value({3,2})==0&&editor.Redo()&&value({3,2})==.5,"Data patch shares document undo/redo");
    editor.ConfigureGesture(0,1,TilemapPaintShape::Rectangle);stroke({6,6},{8,8});Check(value({7,7})==.5,"Rectangle uses same effect");
    editor.ConfigureGesture(0,1,TilemapPaintShape::Circle);stroke({3,8},{4,8});Check(value({3,9})==.5&&value({4,9})==0,"Circle uses same effect");
    editor.HandleEvent(down,GridCoordinate{1,1},true);Check(editor.HasPointerCapture(),"Begin stroke");
    Check(!editor.Preview().dataChanges.empty()&&value({1,1})==0,"Preview does not mutate data");editor.HandleEvent(escape);
    Check(!editor.HasPointerCapture()&&value({1,1})==0&&MoistureBrush::cancels>0,"Escape cancels custom effect");
    editor.SetBounds(Rectanglef{{2,2},{2,2}});editor.ConfigureGesture(0,1,TilemapPaintShape::Rectangle);stroke({2,2},{11,11});
    Check(value({3,3})==.5&&value({4,4})==0,"Bounds owned by engine");
    editor.SetBounds({});Check(editor.Save(),"Save custom data");
    Check(editor.PrepareResize(10,10,1)&&editor.ApplyResize()&&value({7,7})==.5&&editor.Undo(),"Resize preserves numeric layers and undo");
    editor.HandleEvent(down,GridCoordinate{1,1},true);editor.ClearBrushes();
    Check(!editor.HasPointerCapture()&&!editor.ActiveBrush()&&editor.Brushes().empty()&&value({1,1})==0&&MoistureBrush::teardowns>0,"Release plugin objects and cancel before reload");
    Check(editor.Close()&&editor.Open(assets,*id)&&value({7,7})==.5,"Save/reopen retains data");
    auto copy=*editor.Document();TilemapPatch patch;patch.dataTarget="test.moisture";patch.dataChanges={{{0,0},0,.3},{{1,0},.9,.4}};
    Check(!patch.Apply(copy)&&copy.DataLayer("test.moisture")->cells.At({0,0})==0,"Stale data patch fails atomically");
    copy.SetDataLayerLocked("test.moisture",true);patch.dataChanges.resize(1);Check(!patch.Apply(copy),"Locked data cannot be modified");
    std::stringstream invalid;invalid<<"PIPEFRAME_TILEMAP 3\n1 1 1 0 0\n\"\"\n0\n1\n\"Visual\" 1 1 0\n0\n1\n\"test\" 0 1 0\n2\n";
    Check(!TilemapSerializer::Load(invalid,error),"Reject invalid serialized data");
    Check(MoistureBrush::begins>0&&MoistureBrush::ends>0,"Lifecycle hooks execute");
    std::cout<<"Custom brush schema, gestures, lifecycle, data, undo, persistence and atomic validation passed.\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
