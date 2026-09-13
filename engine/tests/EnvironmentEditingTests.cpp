#include <PipeFrame/Editor/TilemapAssetEditor.h>
#include <PipeFrame/Environment/TilemapGeometry.h>
#include <iostream>
using namespace pipeframe;
void Check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    const auto root=std::filesystem::temp_directory_path()/"pipeframe-environment-editing";
    std::filesystem::remove_all(root);std::filesystem::create_directories(root);
    assets::AssetDatabase db;std::string error;Check(db.Open(root,&error),"Open empty project");
    Tilemap2D map(16,12,2);map.AddLayer("Ground");map.DefineTile({1,{220,60,30,255},false});map.DefineTile({2,{20,160,220,255},true});
    {std::ofstream output(root/"Environment.pftilemap");TilemapSerializer::Save(map,output);}
    auto id=db.ImportNow({root/"Environment.pftilemap"},&error);Check(bool(id),"Import blank environment");
    TilemapAssetEditor editor;Check(editor.Open(db,*id),"Open editor");editor.SetEnabled(true);editor.Configure(0,std::make_shared<TileBrush>(1));
    const InputEvent down{InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}},up{InputEventType::PointerReleased,PointerInput{PointerButton::Left,{}}},move{InputEventType::PointerMoved,PointerMoveInput{}};
    const InputEvent escape{InputEventType::KeyPressed,KeyInput{InputKey::Escape}};
    const auto stroke=[&](GridCoordinate a,GridCoordinate b){editor.HandleEvent(down,a,true);editor.HandleEvent(move,b);editor.HandleEvent(up,b);};
    editor.SetBrushOptions(2,false);stroke({3,3},{10,3});
    Check(editor.Document()->Tile(0,{6,1})==1&&editor.Document()->Tile(0,{6,5})==1&&editor.Document()->Tile(0,{6,6})==0,"Round stroke radius with gap-free quick drag");
    Check(editor.Undo()&&!editor.IsDirty()&&editor.Redo(),"Complete radius stroke is one undo command");
    editor.Undo();editor.HandleEvent(down,GridCoordinate{4,4},true);editor.HandleEvent(escape);
    Check(!editor.HasPointerCapture()&&!editor.IsDirty()&&editor.Document()->Tile(0,{4,4})==0,"Escape cancels radius preview");
    editor.SetBounds(Rectanglef{{2.5f,2.5f},{9,9}});editor.SetBrushOptions(0,false);
    editor.Configure(0,std::make_shared<TileBrush>(2),TilemapPaintShape::Selection);
    stroke({2,2},{15,11});auto selected=editor.Selection();
    Check(selected&&selected->minimum==GridCoordinate{2,2}&&selected->maximum==GridCoordinate{5,5}&&!editor.IsDirty(),"Selection clips to partial Playground edge cells without writing");
    Check(editor.PaintSelection(2)&&editor.Document()->Tile(0,{5,5})==2&&editor.Document()->Tile(0,{6,5})==0,"Selection fill is bounded");
    Check(editor.PaintSelection(0)&&editor.Document()->Tile(0,{3,3})==0&&editor.Undo()&&editor.Document()->Tile(0,{3,3})==2,"Selection erase undo restores cells");
    editor.Undo();editor.HandleEvent(escape);Check(!editor.Selection(),"Escape clears selection");
    Check(editor.PaintSelection(2,true)&&editor.Document()->Tile(0,{1,1})==2&&editor.Document()->Tile(0,{5,5})==2&&editor.Document()->Tile(0,{3,3})==0,"Explicit boundary command respects selected bounds");
    Check(editor.Undo()&&!editor.IsDirty(),"Boundary command is one undo transaction");
    editor.SetBounds({});editor.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Line);
    editor.HandleEvent({InputEventType::KeyPressed,KeyInput{InputKey::L}});stroke({2,2},{8,4});
    Check(editor.AxisConstraint()&&editor.Document()->Tile(0,{8,2})==1&&editor.Document()->Tile(0,{8,4})==0,"L constrains line to major axis");
    editor.Undo();editor.Configure(0,std::make_shared<TileBrush>(1));
    editor.HandleEvent(down,GridCoordinate{2,2},true);editor.HandleEvent(move,GridCoordinate{7,3});editor.HandleEvent(up,GridCoordinate{3,9});
    Check(editor.Document()->Tile(0,{6,2})==1&&editor.Document()->Tile(0,{3,7})==0,"Axis-locked pencil keeps the first stroke direction when the pointer crosses axes");
    editor.Undo();editor.SetBrushOptions(0,false);editor.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Ruler);stroke({0,0},{3,4});
    Check(editor.Measurement()&&editor.Measurement()->second==GridCoordinate{3,4}&&!editor.IsDirty(),"Ruler persists after release without recording history");
    Check(editor.EditLayer("add",0,"Details")&&editor.ActiveLayer()==1,"Create layer and make it active");
    Check(editor.EditLayer("rename",1,"Obstacles")&&editor.EditLayer("collision",1)&&!editor.Document()->Layers()[1].collision,"Rename and change collision flag");
    editor.Configure(1,std::make_shared<TileBrush>(2));stroke({4,4},{4,4});
    Check(editor.Document()->Tile(1,{4,4})==2,"Paint active layer");
    editor.HandleEvent(down,GridCoordinate{5,5},true);Check(editor.HasPointerCapture(),"Begin pending stroke");
    Check(editor.EditLayer("lock",1)&&!editor.HasPointerCapture()&&editor.Document()->Tile(1,{5,5})==0,"Locking cancels pending stroke");
    stroke({5,5},{5,5});Check(editor.Document()->Tile(1,{5,5})==0&&!editor.LastError().empty(),"Locked layer rejects painting");
    editor.Configure(1,std::make_shared<TileBrush>(1),TilemapPaintShape::Eyedropper);stroke({4,4},{4,4});Check(editor.SampledTile()==2,"Eyedropper can inspect locked layer");
    Check(editor.EditLayer("lock",1)&&editor.EditLayer("visible",1),"Unlock and hide");editor.Configure(1,std::make_shared<TileBrush>(2));stroke({5,5},{5,5});Check(editor.Document()->Tile(1,{5,5})==0,"Hidden layers reject accidental painting");
    Check(editor.EditLayer("visible",1)&&editor.EditLayer("down",1)&&editor.ActiveLayer()==0&&editor.Document()->Tile(0,{4,4})==2,"Layer reorder moves data and active index");
    Check(editor.EditLayer("remove",0)&&editor.Document()->Layers().size()==1&&editor.Undo()&&editor.Document()->Tile(0,{4,4})==2,"Remove-layer undo restores data");
    Check(editor.DefineTile(2,{10,20,30,255},false)&&!editor.Document()->Definition(2)->solid&&editor.Undo()&&editor.Document()->Definition(2)->solid,"Tile visual/collision metadata uses document undo");
    Check(editor.Save()&&!editor.IsDirty(),"Save environment document");
    TilemapAssetEditor reopened;Check(reopened.Open(db,*id)&&reopened.Document()->Layers()[0].name=="Obstacles"&&!reopened.Document()->Layers()[0].collision&&reopened.Document()->Tile(0,{4,4})==2,"Reopen preserves order, names, flags and data");
    Tilemap2D layered(2,2);layered.DefineTile({1,{255,0,0,255},false});layered.DefineTile({2,{0,0,255,255},true});layered.AddLayer("Lower");layered.AddLayer("Upper");layered.SetTile(0,{0,0},1);layered.SetTile(1,{0,0},2);
    TilemapPatch patch;patch.layer=1;patch.changes.push_back({{0,0},2,0});auto preview=BuildTilemapPreviewGeometry(layered,patch);
    Check(preview.vertices.size()==6&&preview.vertices.front().color==Color{255,0,0,255},"Erase preview reveals lower layer");
    patch.layer=0;patch.changes.front()={{0,0},1,2};preview=BuildTilemapPreviewGeometry(layered,patch);
    Check(preview.vertices.size()==12&&preview.vertices.back().color==Color{0,0,255,255},"Lower-layer paint preview retains upper layer composition");
    std::cout<<"Environment gestures, selection, layers, metadata, undo, persistence and previews passed.\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
