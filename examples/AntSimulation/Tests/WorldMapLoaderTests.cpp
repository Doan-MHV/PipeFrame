#include "Editor/AntFoodBrush.h"
#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/WorldMapLoader.h"
#include <PipeFrame/Environment/TilemapSerializer.h>
#include <PipeFrame/Environment/TilemapEdit.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>

void Require(bool condition,const char *message) {
    if(!condition){std::cerr<<"FAILED: "<<message<<'\n';std::exit(1);}
}
int main() {
    using namespace ant_simulation;
    using namespace pipeframe;
    AntConfiguration configuration;AntEnvironment environment;std::string error;
    Require(environment.Initialize(configuration,error),"Initialize environment");
    Tilemap2D map(10,8);map.DefineTile({1,{45,42,38,255},true});map.AddLayer("Terrain");
    map.SetTile(0,{4,4},1);map.SetTile(0,{6,4},1);
    map.AddDataLayer("ant.food-density",0,10000);
    map.SetData("ant.food-density",{5,4},6);map.SetData("ant.food-density",{6,4},10);
    WorldMapLoadResult result;
    Require(WorldMapLoader::LoadMap(environment,map,"memory.pftilemap",result,error),"Load tilemap");
    Require(result.mapSize==Vector2u{10,8}&&result.addedWallCells==2,"Tilemap dimensions and walls");
    Require(result.addedFoodCells==1&&result.addedFoodQuantity==6,"Density supplies exact quantity, not a magic food ID");
    Require(environment.TryGetCell(6,4)->wall&&environment.TryGetCell(6,4)->foodQuantity==0,"Wall takes priority over density");
    Require(environment.TryGetCell(0,0)->wall,"Protected border remains solid");
    AntFoodBrush brush;
    Require(brush.InitialData(map,{5,4})==0,"New food data starts empty without legacy tile conversion");
    Require(!brush.SetSetting("quantity",std::int64_t{-1},error),"Schema rejects negative quantities");
    Require(brush.SetSetting("quantity",std::int64_t{42},error),"Schema accepts food quantity");
    TilemapEdit paint(map,0);paint.Rectangle({5,4},{5,4},brush);Require(paint.Commit(map),"Paint food data");
    Require(WorldMapLoader::LoadMap(environment,map,{},result,error)&&environment.TryGetCell(5,4)->foodQuantity==42,"Paint reaches runtime");
    brush.erase=true;TilemapEdit erase(map,0);erase.Rectangle({5,4},{5,4},brush);Require(erase.Commit(map),"Erase food data");
    Require(WorldMapLoader::LoadMap(environment,map,{},result,error)&&environment.GetTotalFoodQuantity()==0,"Erased food cannot return from visual tiles");
    const auto path=std::filesystem::temp_directory_path()/"AntWorldMapLoaderTest.pftilemap";
    {std::ofstream out(path);Require(TilemapSerializer::Save(map,out),"Save map");}
    Require(WorldMapLoader::LoadMapFromFile(environment,path,result,error)&&environment.GetTotalFoodQuantity()==0,"Erase survives save/reload");
    std::filesystem::remove(path);
    Require(!WorldMapLoader::ReadMap("obsolete.png",error)&&!error.empty(),"PNG environments are not supported");
    Tilemap2D invalid(10,8,2);invalid.AddLayer("Terrain");const auto walls=environment.GetWallCount();
    Require(!WorldMapLoader::LoadMap(environment,invalid,{},result,error)&&environment.GetWallCount()==walls,"Invalid geometry preserves current world");
    map.SetData("ant.food-density",{5,4},1.5);
    Require(!WorldMapLoader::LoadMap(environment,map,{},result,error),"Fractional food rejected");
    const auto assets=std::filesystem::path(__FILE__).parent_path().parent_path()/"Assets";
    auto shipped=WorldMapLoader::ReadMap(assets/"Tilemaps/Main.pftilemap",error);
    Require(shipped.has_value()&&shipped->DataLayer("ant.food-density"),"Shipped map uses density");
    for(const auto &layer:shipped->Layers())Require(layer.name!="Food","No legacy Food tile layer in shipped map");
    Require(!std::filesystem::exists(assets/"Maps/Main.png"),"No legacy PNG environment asset");
    Require(WorldMapLoader::LoadMap(environment,*shipped,{},result,error),"Load shipped editor map");
    for(int y=2;y<shipped->Rows()-2;++y)for(int x=2;x<shipped->Columns()-2;++x){
        auto *cell=environment.TryGetCell(x,y);
        Require(cell->wall==shipped->IsSolid({x,y}),"Every runtime wall matches authored collision");
        const auto quantity=shipped->IsSolid({x,y})?0:std::size_t(shipped->DataLayer("ant.food-density")->cells.At({x,y}));
        Require(cell->foodQuantity==quantity,"Every runtime food quantity matches authored density");
    }
    std::cout<<"All Ant tilemap and food-density tests passed.\n";
}
