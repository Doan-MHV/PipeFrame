#pragma once
#include <PipeFrame/Environment/Tilemap2D.h>
#include <PipeFrame/Environment/TilemapEdit.h>
#include <PipeFrame/Render/RenderTypes.h>
#include <span>
#include <PipeFrame/Environment/VisualAssets2D.h>
#include <stdexcept>

namespace pipeframe {
// Local-space color geometry with optional atlas UVs. Cache by map and atlas revision.
// Hosts apply object transforms and material state after clipping.
// Rebuild only changed cells, compositing every visible layer. Erasing reveals the
// lower layers; painting a lower layer never covers an unchanged higher layer.
inline GeometryCommand BuildTilemapPreviewGeometry(const Tilemap2D &map,const TilemapPatch &patch,const Tileset2D *atlas=nullptr){
    GeometryCommand result;result.topology=PrimitiveTopology::Triangles;result.space=RenderSpace::World;
    for(const auto &change:patch.changes)for(std::size_t layer=0;layer<map.Layers().size();++layer){
        if(!map.Layers()[layer].visible)continue;
        const auto id=layer==patch.layer?change.after:map.Tile(layer,change.cell);
        const auto *tile=map.Definition(id);if(!tile)continue;
        auto region=atlas?atlas->Region(id):std::optional<Rectanglef>{};
        const auto uv=region?region->position:Vector2f{},extent=region?region->size:Vector2f{};
        const auto p=map.Origin()+Vector2f{float(change.cell.column),float(change.cell.row)}*map.CellSize();const auto size=map.CellSize();
        const Vertex2D a{p,tile->tint,uv},b{p+Vector2f{size,0},tile->tint,uv+Vector2f{extent.x,0}},
            c{p+Vector2f{size,size},tile->tint,uv+extent},d{p+Vector2f{0,size},tile->tint,uv+Vector2f{0,extent.y}};
        result.vertices.insert(result.vertices.end(),{a,b,c,a,c,d});
    }
    return result;
}
inline GeometryCommand BuildTilemapGeometry(const Tilemap2D &map,GridCellRange visible,const Tileset2D *tileset=nullptr) {
    GeometryCommand geometry;geometry.topology=PrimitiveTopology::Triangles;geometry.space=RenderSpace::World;
    const int left=std::max(0,visible.minimum.column),right=std::min(map.Columns()-1,visible.maximum.column);
    const int top=std::max(0,visible.minimum.row),bottom=std::min(map.Rows()-1,visible.maximum.row);
    for(const auto &layer:map.Layers())if(layer.visible)
        for(int y=top;y<=bottom;++y)for(int x=left;x<=right;++x){
            const auto *tile=map.Definition(layer.cells.At({x,y}));if(!tile)continue;
            const auto p=map.Origin()+Vector2f{float(x),float(y)}*map.CellSize();
            const auto size=map.CellSize();
            const auto region=tileset?tileset->Region(tile->id):std::optional<Rectanglef>{};
            const Vector2f uv=region?region->position:Vector2f{},extent=region?region->size:Vector2f{};
            const Vertex2D a{p,tile->tint,uv},b{{p.x+size,p.y},tile->tint,uv+Vector2f{extent.x,0}},
                           c{{p.x+size,p.y+size},tile->tint,uv+extent},d{{p.x,p.y+size},tile->tint,uv+Vector2f{0,extent.y}};
            geometry.vertices.insert(geometry.vertices.end(),{a,b,c,a,c,d});
        }
    return geometry;
}
// Clips the plain-color quads emitted by BuildTilemapGeometry. The source stays
// unchanged so resizing a Playground never crops shared authored cell data.
inline void ClipTilemapGeometry(std::span<const Vertex2D> source,Rectanglef bounds,
                                std::vector<Vertex2D> &output) {
    if(source.size()%6!=0)throw std::invalid_argument("Tilemap geometry must contain six vertices per cell");
    output.clear();
    if(bounds.size.x<=0 || bounds.size.y<=0)return;
    for(std::size_t i=0;i<source.size();i+=6){
        const auto &first=source[i];const auto &opposite=source[i+2];
        const float left=std::max(first.position.x,bounds.position.x);
        const float top=std::max(first.position.y,bounds.position.y);
        const float right=std::min(opposite.position.x,bounds.position.x+bounds.size.x);
        const float bottom=std::min(opposite.position.y,bounds.position.y+bounds.size.y);
        if(left>=right || top>=bottom)continue;
        const auto uv=[&](float x,float y){return Vector2f{
            first.textureCoordinate.x+(opposite.textureCoordinate.x-first.textureCoordinate.x)*(x-first.position.x)/(opposite.position.x-first.position.x),
            first.textureCoordinate.y+(opposite.textureCoordinate.y-first.textureCoordinate.y)*(y-first.position.y)/(opposite.position.y-first.position.y)};};
        const Vertex2D a{{left,top},first.color,uv(left,top)},b{{right,top},first.color,uv(right,top)},
                       c{{right,bottom},first.color,uv(right,bottom)},d{{left,bottom},first.color,uv(left,bottom)};
        output.insert(output.end(),{a,b,c,a,c,d});
    }
}

}
