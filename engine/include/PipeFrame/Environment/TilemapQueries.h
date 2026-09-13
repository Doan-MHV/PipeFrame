#pragma once
#include <PipeFrame/Environment/Tilemap2D.h>
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Spatial/GridRaycast.h>
#include <PipeFrame/Physics/Physics2D.h>

namespace pipeframe {
enum class EnvironmentGeometryKind { Tile, Box, Segment };
struct TilemapQueryHit {
    std::uint64_t objectId{};
    std::size_t layer{};
    GridCoordinate cell{};
    TileId tile{};
    Vector2f point{},normal{};
    float distance{};
    EnvironmentGeometryKind geometry{EnvironmentGeometryKind::Tile};
};
namespace tilemap_query_detail {
inline bool Valid(const Transform2DComponent &t) {
    return std::isfinite(t.position.x)&&std::isfinite(t.position.y)&&std::isfinite(t.rotation)&&
        std::isfinite(t.scale.x)&&std::isfinite(t.scale.y)&&t.scale.x!=0&&t.scale.y!=0;
}
inline Vector2f Rotate(Vector2f p,float angle) {
    const float c=std::cos(angle),s=std::sin(angle);return {p.x*c-p.y*s,p.x*s+p.y*c};
}
inline Vector2f LocalDirection(Vector2f p,const Transform2DComponent &t) {
    p=Rotate(p,-t.rotation);return {p.x/t.scale.x,p.y/t.scale.y};
}
inline std::optional<std::size_t> SolidLayer(const Tilemap2D &map,GridCoordinate cell,std::uint32_t mask) {
    for(std::size_t layer=0;layer<map.Layers().size();++layer){
        if(!(mask&(std::uint32_t{1}<<layer))||!map.Layers()[layer].collision)continue;
        const auto *tile=map.Definition(map.Tile(layer,cell));if(tile&&tile->solid)return layer;
    }
    return std::nullopt;
}
}
// Distances and normals are in world units; masks select tilemap layer indices.
// No cached wall copy: painting, undo and layer changes are observed immediately.
inline std::optional<TilemapQueryHit> RaycastTilemap(const Tilemap2D &map,
    const Transform2DComponent &transform,Vector2f origin,Vector2f direction,float maximumDistance,
    std::uint32_t layerMask=~std::uint32_t{},std::uint64_t objectId=0,
    std::optional<Rectanglef> localBounds={}) {
    using namespace tilemap_query_detail;
    const float length=Length(direction);
    if(!Valid(transform)||!std::isfinite(length)||length<=0||!std::isfinite(maximumDistance)||maximumDistance<0)return {};
    direction/=length;
    auto localOrigin=LocalDirection(origin-transform.position,transform);
    const auto localDirection=LocalDirection(direction,transform);
    const float factor=Length(localDirection);
    float entry=0,end=maximumDistance;Vector2f entryNormal{};
    if(localBounds){
        const auto bounds=*localBounds;
        if(!std::isfinite(bounds.position.x)||!std::isfinite(bounds.position.y)||
           !std::isfinite(bounds.size.x)||!std::isfinite(bounds.size.y)||bounds.size.x<=0||bounds.size.y<=0)return {};
        const auto clip=[&](float p,float d,float low,float high,Vector2f normal){
            if(d==0)return p>=low&&p<high;
            float near=(low-p)/d,far=(high-p)/d;
            if(near>far){std::swap(near,far);normal=-normal;}
            if(near>entry){entry=near;entryNormal=normal;}
            end=std::min(end,far);return entry<=end;
        };
        if(!clip(localOrigin.x,localDirection.x,bounds.position.x,bounds.position.x+bounds.size.x,{-1,0})||
           !clip(localOrigin.y,localDirection.y,bounds.position.y,bounds.position.y+bounds.size.y,{0,-1})||end<0)return {};
        if(entry==0 && ((localOrigin.x>=bounds.position.x+bounds.size.x&&localDirection.x>=0)||
                       (localOrigin.y>=bounds.position.y+bounds.size.y&&localDirection.y>=0)))return {};
        localOrigin+=localDirection*entry;
    }
    const auto hit=RaycastGrid(localOrigin,localDirection,(end-entry)*factor,map.Origin(),map.CellSize(),
        map.Columns(),map.Rows(),[&](GridCoordinate cell){
            // As with primitive boxes, a ray leaving an occupied cell face does
            // not report that cell. The adjacent cell can still hit at distance zero.
            const auto face=map.Origin()+Vector2f{float(cell.column),float(cell.row)}*map.CellSize();
            if((localOrigin.x==face.x&&localDirection.x<0)||
               (localOrigin.x==face.x+map.CellSize()&&localDirection.x>0)||
               (localOrigin.y==face.y&&localDirection.y<0)||
               (localOrigin.y==face.y+map.CellSize()&&localDirection.y>0))return false;
            if(localBounds){
                const auto a=map.Origin()+Vector2f{float(cell.column),float(cell.row)}*map.CellSize();
                const auto end=localBounds->position+localBounds->size;
                if(a.x>=end.x||a.y>=end.y||a.x+map.CellSize()<=localBounds->position.x||
                   a.y+map.CellSize()<=localBounds->position.y)return false;
            }
            return SolidLayer(map,cell,layerMask).has_value();
        });
    if(!hit)return {};
    const auto layer=*SolidLayer(map,hit->cell,layerMask);
    const float distance=entry+hit->distance/factor;
    auto localNormal=Vector2f{float(hit->normal.x),float(hit->normal.y)};
    if(hit->distance==0 && entry>0)localNormal=entryNormal;
    auto normal=Rotate({localNormal.x/transform.scale.x,localNormal.y/transform.scale.y},transform.rotation);
    if(LengthSquared(normal)>0)normal=NormalizeOr(normal);
    return TilemapQueryHit{objectId,layer,hit->cell,map.Tile(layer,hit->cell),origin+direction*distance,normal,distance};
}

// Returns touching/overlapping solid cells. Geometry uses rotated rectangles with
// independent signed axis scales, matching tile rendering (not a scaled circle).
inline std::vector<TilemapQueryHit> OverlapCircleTilemap(const Tilemap2D &map,
    const Transform2DComponent &transform,Circle2D circle,
    std::uint32_t layerMask=~std::uint32_t{},std::uint64_t objectId=0,
    std::optional<Rectanglef> localBounds={}) {
    using namespace tilemap_query_detail;
    std::vector<TilemapQueryHit> result;
    if(!Valid(transform)||!std::isfinite(circle.radius)||circle.radius<0||
       !std::isfinite(circle.center.x)||!std::isfinite(circle.center.y))return result;
    const auto center=Rotate(circle.center-transform.position,-transform.rotation);
    const Vector2f local{center.x/transform.scale.x,center.y/transform.scale.y};
    const Vector2f radius{circle.radius/std::abs(transform.scale.x),circle.radius/std::abs(transform.scale.y)};
    if(!std::isfinite(local.x)||!std::isfinite(local.y)||!std::isfinite(radius.x)||!std::isfinite(radius.y))return result;
    const auto index=[&](double value,double origin,int count){return int(std::clamp(std::floor((value-origin)/map.CellSize()),-1.0,double(count)));};
    const int left=std::max(0,index(std::nextafter(double(local.x)-radius.x,-std::numeric_limits<double>::infinity()),map.Origin().x,map.Columns()));
    const int right=std::min(map.Columns()-1,index(double(local.x)+radius.x,map.Origin().x,map.Columns()));
    const int top=std::max(0,index(std::nextafter(double(local.y)-radius.y,-std::numeric_limits<double>::infinity()),map.Origin().y,map.Rows()));
    const int bottom=std::min(map.Rows()-1,index(double(local.y)+radius.y,map.Origin().y,map.Rows()));
    for(int y=top;y<=bottom;++y)for(int x=left;x<=right;++x){
        const GridCoordinate cell{x,y};const auto layer=SolidLayer(map,cell,layerMask);if(!layer)continue;
        auto a=map.Origin()+Vector2f{float(x),float(y)}*map.CellSize(),b=a+Vector2f{map.CellSize(),map.CellSize()};
        if(localBounds){
            const auto bounds=*localBounds;
            if(!std::isfinite(bounds.position.x)||!std::isfinite(bounds.position.y)||
               !std::isfinite(bounds.size.x)||!std::isfinite(bounds.size.y)||bounds.size.x<=0||bounds.size.y<=0)return {};
            a={std::max(a.x,bounds.position.x),std::max(a.y,bounds.position.y)};
            b={std::min(b.x,bounds.position.x+bounds.size.x),std::min(b.y,bounds.position.y+bounds.size.y)};
            if(a.x>=b.x||a.y>=b.y)continue;
        }
        a={a.x*transform.scale.x,a.y*transform.scale.y};b={b.x*transform.scale.x,b.y*transform.scale.y};
        const Vector2f closest{std::clamp(center.x,std::min(a.x,b.x),std::max(a.x,b.x)),
                               std::clamp(center.y,std::min(a.y,b.y),std::max(a.y,b.y))};
        const float distance=Length(center-closest);if(distance>circle.radius)continue;
        const auto normal=distance>0?Rotate((center-closest)/distance,transform.rotation):Vector2f{};
        result.push_back({objectId,*layer,cell,map.Tile(*layer,cell),
            transform.position+Rotate(closest,transform.rotation),normal,distance});
    }
    return result;
}
}
