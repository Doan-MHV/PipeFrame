#include <PipeFrame/Environment/TilemapEdit.h>
#include <PipeFrame/Editor/TilemapPaintTool.h>
#include <PipeFrame/Environment/TilemapGeometry.h>
#include <PipeFrame/Environment/TilemapQueries.h>
#include <PipeFrame/Environment/TilemapSweep.h>
#include <PipeFrame/Spatial/GridRaycast.h>
#include <PipeFrame/Environment/TilemapSerializer.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
using namespace pipeframe;
void Require(bool value,const char *message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
void TestPaintGestures() {
    {
        Tilemap2D bounded(8,8);bounded.DefineTile({1,{255,0,0,255},true});bounded.AddLayer("Walls");
        TileBrush brush(1);
        TilemapEdit fill(bounded,0,Rectanglef{{1.5f,1.5f},{2,2}});
        fill.Fill({2,2},brush);
        Require(fill.Preview().changes.size()==9,"Bounded fill includes partial edge cells and stops at playground bounds");
        Require(fill.Commit(bounded)&&bounded.Tile(0,{0,2})==0&&bounded.Tile(0,{4,2})==0,
                "Bounded transaction cannot change cells outside playground");
        TilemapPaintTool clipped(bounded);clipped.Configure(0,std::make_shared<TileBrush>(0));
        clipped.SetBounds(Rectanglef{{1.5f,1.5f},{2,2}});clipped.SetEnabled(true);
        InputEvent down{InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}};
        Require(!clipped.HandleEvent(down,GridCoordinate{0,0},true).consumed,"Outside bounded press does not capture");
        Require(clipped.HandleEvent(down,GridCoordinate{2,2},true).consumed,"Inside bounded press captures");
        clipped.SetBounds(Rectanglef{{0,0},{1,1}});
        Require(!clipped.HasPointerCapture()&&bounded.Tile(0,{2,2})==1,"Bounds change cancels active preview");
    }

    Tilemap2D map(10,8); map.DefineTile({1,{255,0,0,255},true}); map.AddLayer("Ground");
    TilemapPaintTool tool(map); tool.Configure(0,std::make_shared<TileBrush>(1)); tool.SetEnabled(true);
    const InputEvent press{InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}};
    const InputEvent release{InputEventType::PointerReleased,PointerInput{PointerButton::Left,{}}};
    const InputEvent move{InputEventType::PointerMoved,PointerMoveInput{}};
    auto at=[](int x,int y){return std::optional<GridCoordinate>{{x,y}};};
    Require(!tool.HandleEvent(press,at(1,1),false).consumed,"UI hit-test must prevent stroke starts");
    Require(!tool.HandleEvent(press,at(-1,1),true).consumed,"Outside press must not capture");
    Require(tool.HandleEvent(press,at(1,1),true).consumed&&tool.HasPointerCapture(),"Valid press captures");
    tool.HandleEvent(move,at(6,1));
    Require(tool.Preview().changes.size()==6&&map.Tile(0,{1,1})==0,"Fast drag interpolates without writing");
    auto result=tool.HandleEvent(release,at(20,1));
    Require(result.committed&&result.committed->changes.size()==9&&!tool.HasPointerCapture(),"Outside release commits one clipped stroke");
    Require(result.committed->Apply(map,false)&&map.Tile(0,{6,1})==0&&result.committed->Apply(map),"Gesture patch supports undo and redo");
    Require(!tool.HandleEvent(release,at(20,1)).committed,"Duplicate release cannot commit twice");
    tool.HandleEvent(press,at(1,1),true);
    Require(!tool.HandleEvent(release,at(1,1)).committed,"No-op click creates no history entry");

    tool.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Rectangle);
    tool.HandleEvent(press,at(0,3),true); tool.HandleEvent(move,at(5,7)); tool.HandleEvent(move,at(1,4));
    Require(tool.Preview().changes.size()==4,"Shrinking rectangle replaces its preview");
    tool.HandleEvent({InputEventType::KeyPressed,KeyInput{InputKey::Escape}});
    Require(!tool.HasPointerCapture()&&map.Tile(0,{0,3})==0,"Escape cancels without mutation");
    for(const auto &event: {InputEvent{InputEventType::FocusChanged,FocusInput{false}},
                           InputEvent{InputEventType::PointerPresenceChanged,PointerPresenceInput{false}}}) {
        tool.HandleEvent(press,at(0,3),true); tool.HandleEvent(event);
        Require(!tool.HasPointerCapture()&&map.Tile(0,{0,3})==0,"Window exit/focus loss cancels");
    }
    tool.HandleEvent(press,at(0,3),true); tool.SetEnabled(false);
    Require(!tool.HasPointerCapture()&&tool.Preview().changes.empty(),"Disabling discards gesture");
    tool.SetEnabled(true); tool.HandleEvent(press,at(0,3),true);
    Require(!tool.HandleEvent(release).committed&&!tool.HasPointerCapture(),"Missing release coordinate cancels");
    tool.HandleEvent(press,at(0,3),true); map.SetTile(0,{9,7},1);
    Require(tool.Preview().changes.empty()&&!tool.HandleEvent(release,at(1,4)).committed&&
            !tool.LastError().empty()&&map.Tile(0,{0,3})==0,"External edit invalidates the whole stroke");
    map.SetLayerFlags(0,true,true,true);
    tool.HandleEvent(press,at(0,3),true);
    Require(!tool.HasPointerCapture()&&!tool.LastError().empty(),"Locked layer fails safely");
    map.SetLayerFlags(0,true,true,false);

    struct Toggle final:EnvironmentBrush {
        TileId Paint(GridCoordinate,TileId old)const override{return old?0:1;}
    };
    tool.Configure(0,std::make_shared<Toggle>());
    tool.HandleEvent(press,at(0,5),true); tool.HandleEvent(move,at(3,5)); tool.HandleEvent(move,at(0,5));
    result=tool.HandleEvent(release,at(0,5));
    Require(result.committed&&result.committed->changes.size()==4,"Custom brush runs once per cell despite retracing/release");
    tool.Configure(0,std::make_shared<TileBrush>(999));
    tool.HandleEvent(press,at(0,6),true);
    Require(!tool.HasPointerCapture()&&!tool.LastError().empty()&&map.Tile(0,{0,6})==0,"Bad custom brush cancels without partial writes");

    tool.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Line);
    tool.HandleEvent(press,at(0,6),true); result=tool.HandleEvent(release,at(2,6));
    Require(result.committed&&result.committed->changes.size()==3,"Line gesture commits its endpoints");
    tool.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Outline);
    tool.HandleEvent(press,at(5,3),true); result=tool.HandleEvent(release,at(7,5));
    Require(result.committed&&result.committed->changes.size()==8&&map.Tile(0,{6,4})==0,"Outline keeps its center empty");
    tool.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Fill);
    tool.HandleEvent(press,at(6,4),true); result=tool.HandleEvent(release,at(0,0));
    Require(result.committed&&result.committed->changes.size()==1,"Fill uses the pressed seed and respects boundaries");
    const auto revision=map.Revision();
    tool.Configure(0,std::make_shared<TileBrush>(0),TilemapPaintShape::Eyedropper);
    tool.HandleEvent(press,at(6,4),true);result=tool.HandleEvent(release,at(6,4));
    Require(tool.SampledTile()==1&&!result.committed&&map.Revision()==revision,"Eyedropper reads without modifying or recording history");
    tool.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Ruler);
    tool.HandleEvent(press,at(0,0),true);tool.HandleEvent(move,at(3,4));
    Require(tool.Measurement()&&tool.Measurement()->second==GridCoordinate{3,4}&&tool.Preview().changes.empty(),"Ruler exposes endpoints without a paint preview");
    result=tool.HandleEvent(release,at(3,4));
    Require(!result.committed&&map.Revision()==revision,"Ruler never mutates map");
    tool.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Circle);
    tool.HandleEvent(press,at(0,0),true);tool.HandleEvent(move,at(1,0));
    Require(!tool.Preview().changes.empty()&&map.Revision()==revision,"Circle previews clipped cells without writing");
    tool.SetEnabled(false);

}
void TestAuthoredQueries() {
    Tilemap2D map(4,4);map.DefineTile({1,{200,100,50,255},true});map.AddLayer("Walls");
    map.AddLayer("Decoration");map.SetLayerFlags(1,true,false,false);
    TilemapEdit edit(map,0);TileBrush wall(1);edit.Sample({1,1},wall);auto patch=edit.Preview();
    Require(edit.Commit(map),"Paint query fixture");
    Transform2DComponent transform;transform.position={10,20};transform.scale={2,3};
    auto hit=RaycastTilemap(map,transform,{0,24},{1,0},50,1,42);
    Require(hit&&hit->cell==GridCoordinate{1,1}&&hit->layer==0&&hit->objectId==42&&
        std::abs(hit->distance-12)<0.001f&&hit->normal==Vector2f{-1,0},"Outside ray returns world distance and stable cell identity");
    Require(!RaycastTilemap(map,transform,{0,24},{1,0},11)&&
            !RaycastTilemap(map,transform,{0,24},{1,0},50,2),"Ray range and collision layer mask filter hits");
    auto overlaps=OverlapCircleTilemap(map,transform,{{11.5f,24},0.5f});
    Require(overlaps.size()==1&&std::abs(overlaps[0].distance-0.5f)<0.001f,"World circle touches transformed tile face");
    Require(OverlapCircleTilemap(map,transform,{{11.4f,24},0.5f}).empty(),"Clearance query rejects separated circle");
    Require(OverlapCircleTilemap(map,transform,{{14.5f,24},0.5f}).size()==1,"Tangency includes previous cell at exact lower search bound");
    transform.rotation=3.14159265358979323846f/2;
    hit=RaycastTilemap(map,transform,{6,10},{0,1},50);
    Require(hit&&std::abs(hit->distance-12)<0.001f&&std::abs(hit->normal.y+1)<0.001f,"Rotated ray normal and distance use world units");
    Require(OverlapCircleTilemap(map,transform,{{6,21.5f},0.51f}).size()==1,"Circle overlap matches rotated rendered rectangle");
    transform.rotation=0;transform.scale={-2,3};
    hit=RaycastTilemap(map,transform,{20,24},{-1,0},50);
    Require(hit&&std::abs(hit->distance-12)<0.001f&&hit->normal.x==1,"Reflected map preserves outward normal");
    Require(patch.Apply(map,false)&&!RaycastTilemap(map,transform,{20,24},{-1,0},50),"Undo immediately removes query obstacle");
    Require(patch.Apply(map),"Redo restores obstacle");
    map.SetLayerFlags(0,false,true,false);
    Require(RaycastTilemap(map,transform,{20,24},{-1,0},50).has_value(),"Hidden tiles still collide when collision enabled");
    map.SetLayerFlags(0,true,false,false);
    Require(!RaycastTilemap(map,transform,{20,24},{-1,0},50),"Disabling collision immediately updates query");
    transform.scale.x=0;
    Require(OverlapCircleTilemap(map,transform,{{10,20},1}).empty(),"Degenerate transform is rejected");
    map.SetLayerFlags(0,true,true,false);transform={};
    auto swept=SweepCircleTilemap(map,transform,{{0,1.5f},.25f},{100,0});
    Require(swept&&std::abs(swept->distance-.75f)<.001f,"Continuous sweep catches wall across long movement");
    Require(!SweepCircleTilemap(map,transform,{{.75f,1.5f},.25f},{-2,0}),"Touching body can move away");
    swept=SweepCircleTilemap(map,transform,{{0,0},.25f},{2,2});
    Require(swept&&std::abs(swept->distance-(std::sqrt(2.f)-.25f))<.001f,"Rounded corner sweep uses actual circle geometry");
    Require(!SweepCircleTilemap(map,transform,{{0,.74f},.25f},{3,0}),"Sweep does not collide with inflated square corner false positives");
    transform.rotation=1.57079632679f;transform.scale={2,3};
    swept=SweepCircleTilemap(map,transform,{{-4.5f,0},.25f},{0,100});
    Require(swept&&std::abs(swept->distance-1.75f)<.001f,"Sweep respects rotation and nonuniform tile scale");
    const auto solid=[](GridCoordinate){return true;};
    Require(!RaycastGrid({8,1},{1,0},10,{0,0},1,4,4,solid),"Outside ray pointing away misses");
    auto edge=RaycastGrid({4,1},{-1,0},10,{0,0},1,4,4,solid);
    Require(edge&&edge->cell.column==3&&edge->distance==0,"Ray on maximum boundary pointing inward enters final cell");
}
int main(){
    TestPaintGestures();
    TestAuthoredQueries();
    Tilemap2D map(10,8,2,{-10,-8});map.DefineTile({1,{100,80,60,255},true});map.DefineTile({2,{0,200,0,255},false});
    const auto layer=map.AddLayer("Ground");TileBrush wall(1),floor(2),erase(0);
    Require(map.CellAt({-10,-8})==GridCoordinate{0,0} && !map.CellAt({10,0}) && !map.CellAt({-11,0}),"Local picking must clip at map bounds");
    Require(!map.IsSolid({-1,0})&&!map.IsSolid({0,0}),"Bounds must not create walls");
    TilemapEdit stroke(map,layer);stroke.Line({-1000000,2},{1000000,2},wall);
    auto patch=stroke.Preview();Require(patch.changes.size()==10&&!map.IsSolid({3,2}),"Preview clips without mutating the map");
    Require(stroke.Commit(map)&&map.IsSolid({3,2}),"Stroke commit must update shared collision data");
    Require(!stroke.Commit(map),"Stale gesture must not commit twice");
    Require(patch.Apply(map,false)&&!map.IsSolid({3,2})&&patch.Apply(map),"Whole stroke undo and redo");
    TilemapEdit flood(map,layer);flood.Fill({0,0},floor);Require(flood.Preview().changes.size()==20&&flood.Commit(map),"Fill must stop at a wall");
    TilemapEdit cancelled(map,layer);cancelled.Rectangle({-5,-5},{1,1},erase);cancelled.Cancel();
    Require(cancelled.Preview().changes.empty()&&map.Tile(layer,{0,0})==2,"Cancel must discard preview");
    TilemapEdit rectangle(map,layer);rectangle.Rectangle({8,6},{20,20},wall);
    Require(rectangle.Preview().changes.size()==4&&rectangle.Commit(map),"Rectangle clips to selected map");
    map.SetLayerFlags(layer,false,true,false);Require(map.IsSolid({3,2}),"Hidden layer may still collide");
    map.SetLayerFlags(layer,true,false,true);Require(!map.IsSolid({3,2})&&!patch.Apply(map,false),"Collision and edit lock are independent");
    bool locked=false;try{TilemapEdit edit(map,layer);}catch(const std::invalid_argument&){locked=true;}
    Require(locked,"Locked layers reject gestures");
    std::stringstream saved;Require(TilemapSerializer::Save(map,saved),"Serialize tilemap");std::string error;
    auto loaded=TilemapSerializer::Load(saved,error);Require(loaded.has_value(),"Reload tilemap");
    std::stringstream roundTrip;Require(TilemapSerializer::Save(*loaded,roundTrip)&&roundTrip.str()==saved.str(),"Deterministic tile/flags/origin round trip");
    std::stringstream truncated(saved.str().substr(0,saved.str().size()/2));Require(!TilemapSerializer::Load(truncated,error),"Reject truncated data");
    std::stringstream huge("PIPEFRAME_TILEMAP 1\n2147483647 2147483647 1 0 0\n");Require(!TilemapSerializer::Load(huge,error),"Reject excessive allocation before creating a grid");
    // Developer-defined effects use the same clipping, preview and patch machinery.
    struct CheckerBrush final:EnvironmentBrush{TileId Paint(GridCoordinate cell,TileId previous)const override{return (cell.column+cell.row)%2?previous:1;}} checker;
    Tilemap2D independent(3,3);independent.DefineTile({1,{255,0,0,255},true});independent.AddLayer("Robot obstacles");
    TilemapEdit custom(independent,0);custom.Rectangle({0,0},{2,2},checker);Require(custom.Preview().changes.size()==5&&custom.Commit(independent),"Custom brush shares engine transactions");
    const auto geometry=BuildTilemapGeometry(independent,{{0,0},{2,2}});
    Require(geometry.vertices.size()==30 && independent.IsSolid({0,0}),"Renderer and collision must consume the painted cells");
    const auto hit=RaycastGrid({1.5f,0.5f},{1,0},10,independent.Origin(),independent.CellSize(),
        independent.Columns(),independent.Rows(),[&](GridCoordinate cell){return independent.IsSolid(cell);});
    Require(hit && hit->cell==GridCoordinate{2,0} && hit->distance==0.5f,"Existing grid ray query reads authored solidity");
    Require(BuildTilemapGeometry(independent,{{1,1},{1,1}}).vertices.size()==6,"Visible cell range bounds geometry generation");
    independent.SetLayerFlags(0,false,true,false);
    Require(BuildTilemapGeometry(independent,{{0,0},{2,2}}).vertices.empty()&&independent.IsSolid({0,0}),"Hiding graphics must not remove collision");
    TilemapEdit noOp(independent,0);noOp.Fill({0,0},wall);Require(noOp.Preview().changes.empty(),"No-op fill terminates without recording edits");
    TilemapEdit diagonal(independent,0);diagonal.Line({-9,-9},{9,9},erase);
    Require(diagonal.Preview().changes.size()==3,"Clipped diagonal retains its slope");
    TilemapEdit circle(independent,0);circle.Circle({0,0},1,wall);
    Require(circle.Preview().changes.size()==2,"Circular brush clips its footprint at the map boundary");
    std::vector<Vertex2D> clipped;
    ClipTilemapGeometry(geometry.vertices,{{0.5f,0.5f},{1,1}},clipped);
    Require(clipped.size()==12,"Partial corner cells remain visible inside local bounds");
    for(const auto &vertex:clipped)Require(vertex.position.x>=0.5f&&vertex.position.x<=1.5f&&
        vertex.position.y>=0.5f&&vertex.position.y<=1.5f,"Clipped vertices remain inside Playground");
    ClipTilemapGeometry(geometry.vertices,{{50,50},{1,1}},clipped);
    Require(clipped.empty(),"Fully outside tiles do not draw");
    Require(independent.Tile(0,{0,0})==1,"Clipping must not alter shared tile cells");
    std::cout<<"Tilemap regression passed\n";
}
