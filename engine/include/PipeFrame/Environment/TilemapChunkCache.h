#pragma once
#include <PipeFrame/Environment/TilemapGeometry.h>
#include <cstring>
namespace pipeframe {
// Fingerprints inspect authored cells only after an edit/reimport. Unchanged chunk
// vertex buffers survive, including edits to unrelated numeric brush data layers.
class TilemapChunkCache {
public:
    static constexpr int ChunkSize=32;
    struct Chunk { GridCellRange cells;std::uint64_t fingerprint{};GeometryCommand geometry; };
    void Update(const Tilemap2D &map,const Tileset2D *atlas=nullptr,bool newDocument=false){
        std::optional<Tileset2D> desired=atlas?std::optional{*atlas}:std::nullopt;
        rebuilt=0;
        if(!newDocument&&revision==map.Revision()&&tileset==desired)return;
        const bool layoutChanged=columns!=map.Columns()||rows!=map.Rows()||cellSize!=map.CellSize()||origin!=map.Origin()||tileset!=desired;
        if(layoutChanged)chunks.clear();
        columns=map.Columns();rows=map.Rows();cellSize=map.CellSize();origin=map.Origin();tileset=desired;
        std::size_t index=0;
        for(int y=0;y<rows;y+=ChunkSize)for(int x=0;x<columns;x+=ChunkSize,++index){
            const GridCellRange cells{{x,y},{std::min(columns-1,x+ChunkSize-1),std::min(rows-1,y+ChunkSize-1)}};
            std::uint64_t hash=1469598103934665603ULL;
            const auto mix=[&](std::uint64_t value){hash^=value;hash*=1099511628211ULL;};
            for(const auto &layer:map.Layers()){
                mix(layer.visible);if(!layer.visible)continue;
                for(int cy=cells.minimum.row;cy<=cells.maximum.row;++cy)for(int cx=cells.minimum.column;cx<=cells.maximum.column;++cx){
                    const auto id=layer.cells.At({cx,cy});mix(id);
                    if(const auto *tile=map.Definition(id)){mix(tile->tint.r);mix(tile->tint.g);mix(tile->tint.b);mix(tile->tint.a);}
                }
            }
            if(index==chunks.size())chunks.push_back({cells,~hash,{}});
            auto &chunk=chunks[index];
            if(chunk.fingerprint!=hash){chunk.fingerprint=hash;chunk.geometry=BuildTilemapGeometry(map,cells,atlas);++rebuilt;}
        }
        revision=map.Revision();
    }
    const auto &Chunks()const{return chunks;}
    std::size_t RebuiltChunks()const{return rebuilt;}
    // Reuse the host buffer and reserve once: rebuilding a flattened map must not
    // repeatedly grow/copy a multi-megabyte vector while the previous one is alive.
    void Flatten(std::vector<Vertex2D> &vertices)const{
        std::size_t count=0;for(const auto &chunk:chunks)count+=chunk.geometry.vertices.size();
        vertices.clear();
        if(count>vertices.capacity())vertices.reserve(std::max(count,vertices.capacity()+vertices.capacity()/2));
        for(const auto &chunk:chunks)vertices.insert(vertices.end(),chunk.geometry.vertices.begin(),chunk.geometry.vertices.end());
    }
    GeometryCommand Flatten()const{
        GeometryCommand result;result.topology=PrimitiveTopology::Triangles;result.space=RenderSpace::World;
        Flatten(result.vertices);return result;
    }

private:
    int columns{},rows{};float cellSize{};Vector2f origin{};
    std::uint64_t revision{~std::uint64_t{}};
    std::optional<Tileset2D> tileset;
    std::vector<Chunk> chunks;std::size_t rebuilt{};
};
}
