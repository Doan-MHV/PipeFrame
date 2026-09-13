#pragma once
#include <PipeFrame/Data/Grid2D.h>
#include <PipeFrame/Foundation/MathTypes.h>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace pipeframe {
using TileId = std::uint32_t;
struct TileDefinition2D {
    TileId id{}; // Zero is empty and cannot be registered.
    Color tint{255,255,255,255};
    bool solid{};
};
struct DataLayer2D {
    std::string id;
    double minimum{}, maximum{1};
    bool locked{};
    Grid2D<double> cells;
};
struct TileLayer2D {
    std::string name;
    bool visible{true}, collision{true}, locked{};
    Grid2D<TileId> cells;
};
// Authored local-space data; rendering and collision read the same tile definitions.
class Tilemap2D {
public:
    static constexpr std::size_t MaxCells = 4 * 1024 * 1024;
    Tilemap2D(int columns, int rows, float cellSize=1, Vector2f origin={})
        : columns(columns), rows(rows), cellSize(cellSize), origin(origin) {
        if(columns<=0 || rows<=0 || std::size_t(columns)>MaxCells/std::size_t(rows) ||
           !std::isfinite(cellSize) || cellSize<=0 || !std::isfinite(origin.x) || !std::isfinite(origin.y) ||
           !std::isfinite(origin.x+columns*cellSize) || !std::isfinite(origin.y+rows*cellSize))
            throw std::invalid_argument("Invalid tilemap dimensions or cell size");
    }
    void ReplaceWith(const Tilemap2D &other) {const auto next=std::max(revision,other.revision)+1;*this=other;revision=next;}
    const std::string &Tileset() const { return tileset; }
    void SetTileset(std::string id) { if(tileset!=id){tileset=std::move(id);++revision;} }
    int Columns() const { return columns; }
    int Rows() const { return rows; }
    float CellSize() const { return cellSize; }
    Vector2f Origin() const { return origin; }
    Rectanglef Bounds() const { return {origin,{columns*cellSize,rows*cellSize}}; }
    std::uint64_t Revision() const { return revision; }
    bool Contains(GridCoordinate cell) const { return cell.column>=0 && cell.row>=0 && cell.column<columns && cell.row<rows; }
    std::optional<GridCoordinate> CellAt(Vector2f local) const {
        if(!std::isfinite(local.x) || !std::isfinite(local.y) || !Bounds().Contains(local))return std::nullopt;
        GridCoordinate cell{int(std::floor((local.x-origin.x)/cellSize)),int(std::floor((local.y-origin.y)/cellSize))};
        return Contains(cell) ? std::optional{cell} : std::nullopt;
    }
    void DefineTile(TileDefinition2D tile) {
        if(tile.id==0)throw std::invalid_argument("Tile zero is reserved for empty cells");
        definitions.insert_or_assign(tile.id,tile); ++revision;
    }
    const auto &Definitions() const { return definitions; }
    const TileDefinition2D *Definition(TileId id) const {
        const auto found=definitions.find(id);return found==definitions.end()?nullptr:&found->second;
    }
    std::size_t AddLayer(std::string name) {
        if(name.empty() || layers.size()+dataLayers.size()>=32 || std::size_t(columns)*rows>MaxCells/(layers.size()+dataLayers.size()+1))
            throw std::invalid_argument("Invalid layer name or tilemap cell budget exceeded");
        layers.push_back({std::move(name),true,true,false,Grid2D<TileId>(columns,rows)}); ++revision;
        return layers.size()-1;
    }
    const auto &DataLayers() const { return dataLayers; }
    const DataLayer2D *DataLayer(std::string_view id) const {
        for(const auto &layer:dataLayers)if(layer.id==id)return &layer;
        return nullptr;
    }
    void AddDataLayer(std::string id,double minimum=0,double maximum=1) {
        if(id.empty()||id.size()>128||DataLayer(id)||!std::isfinite(minimum)||!std::isfinite(maximum)||minimum>0||maximum<0||minimum>=maximum||
           layers.size()+dataLayers.size()>=32||std::size_t(columns)*rows>MaxCells/(layers.size()+dataLayers.size()+1))
            throw std::invalid_argument("Invalid data layer or cell budget exceeded");
        dataLayers.push_back({std::move(id),minimum,maximum,false,Grid2D<double>(columns,rows)});++revision;
    }
    bool SetData(std::string_view id,GridCoordinate cell,double value) {
        for(auto &layer:dataLayers)if(layer.id==id){
            if(layer.locked||!Contains(cell)||!std::isfinite(value)||value<layer.minimum||value>layer.maximum)return false;
            if(layer.cells.At(cell)!=value){layer.cells.At(cell)=value;++revision;}return true;
        }return false;
    }
    void SetDataLayerLocked(std::string_view id,bool locked) {
        for(auto &layer:dataLayers)if(layer.id==id){layer.locked=locked;++revision;return;}
        throw std::invalid_argument("Unknown data layer");
    }
    const auto &Layers() const { return layers; }
    void RenameLayer(std::size_t index,std::string name) {
        if(name.empty()||name.size()>80)throw std::invalid_argument("Layer name must contain 1–80 characters");
        layers.at(index).name=std::move(name);++revision;
    }
    void RemoveLayer(std::size_t index) {
        if(index>=layers.size()||layers.size()==1)throw std::invalid_argument("Keep at least one layer");
        layers.erase(layers.begin()+index);++revision;
    }
    void MoveLayer(std::size_t index,std::size_t destination) {
        if(index>=layers.size()||destination>=layers.size())throw std::invalid_argument("Invalid layer order");
        auto layer=std::move(layers[index]);layers.erase(layers.begin()+index);
        layers.insert(layers.begin()+destination,std::move(layer));++revision;
    }
    void SetLayerFlags(std::size_t index,bool visible,bool collision,bool locked) {
        auto &layer=layers.at(index);layer.visible=visible;layer.collision=collision;layer.locked=locked;++revision;
    }
    TileId Tile(std::size_t layer,GridCoordinate cell) const { return layers.at(layer).cells.At(cell); }
    bool SetTile(std::size_t layer,GridCoordinate cell,TileId tile) {
        auto &target=layers.at(layer);
        if(target.locked || !Contains(cell) || (tile!=0 && !Definition(tile)))return false;
        if(target.cells.At(cell)!=tile){target.cells.At(cell)=tile;++revision;}
        return true;
    }
    bool IsSolid(GridCoordinate cell) const {
        if(!Contains(cell))return false; // Bounds are not implicit walls.
        for(const auto &layer:layers)if(layer.collision)
            if(const auto *tile=Definition(layer.cells.At(cell));tile && tile->solid)return true;
        return false;
    }
private:
    std::string tileset;
    int columns,rows;
    float cellSize;
    Vector2f origin;
    std::uint64_t revision{};
    std::unordered_map<TileId,TileDefinition2D> definitions;
    std::vector<TileLayer2D> layers;
    std::vector<DataLayer2D> dataLayers;
};
}
