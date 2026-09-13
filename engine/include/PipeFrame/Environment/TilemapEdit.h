#pragma once
#include <PipeFrame/Environment/Tilemap2D.h>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <optional>

namespace pipeframe {
// Brushes supply an effect; the transaction owns deduplication and undo data.
class EnvironmentBrush {
public:
    virtual ~EnvironmentBrush()=default;
    // Observational hooks: effects never mutate the document or own undo commands.
    virtual void OnActivate()const noexcept{}
    virtual void OnBegin(GridCoordinate)const noexcept{}
    virtual void OnUpdate(GridCoordinate)const noexcept{}
    virtual void OnPreview(std::size_t)const noexcept{}
    virtual void OnEnd()const noexcept{}
    virtual void OnCancel()const noexcept{}
    virtual void OnDeactivate()const noexcept{}
    virtual void OnTeardown()const noexcept{}
    virtual TileId Paint(GridCoordinate, TileId previous) const{return previous;}
    // Empty target selects visual tiles. A named target edits independent numeric data.
    virtual std::string_view DataTarget() const{return {};}
    virtual double PaintData(GridCoordinate, double previous) const{return previous;}
};
class TileBrush final : public EnvironmentBrush {
public:
    explicit TileBrush(TileId tile):tile(tile){}
    TileId Paint(GridCoordinate,TileId)const override{return tile;}
private:TileId tile;
};
struct TileChange { GridCoordinate cell; TileId before{},after{}; };
struct DataChange { GridCoordinate cell; double before{},after{}; };
class TilemapPatch {
public:
    std::size_t layer{};
    std::vector<TileChange> changes;
    std::string dataTarget;
    std::vector<DataChange> dataChanges;
    bool Empty()const{return changes.empty()&&dataChanges.empty();}
    // Validate the entire patch before writing: no partial undo on stale data.
    bool Apply(Tilemap2D &map,bool forward=true) const {
        if(!dataTarget.empty()){
            if(!changes.empty())return false;
            const auto *target=map.DataLayer(dataTarget);
            if(!target||target->locked)return false;
            std::set<std::pair<int,int>> seen;
            for(const auto &change:dataChanges){
                const double value=forward?change.after:change.before;
                if(!seen.emplace(change.cell.column,change.cell.row).second||!map.Contains(change.cell)||!std::isfinite(value)||value<target->minimum||value>target->maximum||
                    target->cells.At(change.cell)!=(forward?change.before:change.after))return false;
            }
            for(const auto &change:dataChanges)map.SetData(dataTarget,change.cell,forward?change.after:change.before);
            return true;
        }
        if(!dataChanges.empty())return false;
        if(layer>=map.Layers().size() || map.Layers()[layer].locked)return false;
        std::set<std::pair<int,int>> seen;
        for(const auto &change:changes) {
            if(!seen.emplace(change.cell.column,change.cell.row).second)return false;
            const auto target=forward?change.after:change.before;
            if(!map.Contains(change.cell) || (target && !map.Definition(target)) ||
               map.Tile(layer,change.cell)!=(forward?change.before:change.after))return false;
        }
        for(const auto &change:changes)map.SetTile(layer,change.cell,forward?change.after:change.before);
        return true;
    }
};
class TilemapEdit {
public:
    TilemapEdit(const Tilemap2D &map,std::size_t layer,std::optional<Rectanglef> bounds={},bool readOnly=false):map(map),layer(layer),revision(map.Revision()),bounds(bounds) {
        if(layer>=map.Layers().size() || (!readOnly&&map.Layers()[layer].locked))throw std::invalid_argument("Layer is unavailable for painting");
    }
    bool Contains(GridCoordinate cell) const {
        if (!map.Contains(cell)) return false;
        if (!bounds) return true;
        const auto start=map.Origin()+Vector2f{float(cell.column),float(cell.row)}*map.CellSize();
        return bounds->size.x>0 && bounds->size.y>0 &&
            start.x < bounds->position.x+bounds->size.x && start.y < bounds->position.y+bounds->size.y &&
            start.x+map.CellSize()>bounds->position.x && start.y+map.CellSize()>bounds->position.y;
    }
    void Sample(GridCoordinate cell,const EnvironmentBrush &brush) {
        if(!Contains(cell))return;
        const auto key=std::size_t(cell.row)*map.Columns()+cell.column;
        if(!brush.DataTarget().empty()){
            if(!pending.empty()||(!dataTarget.empty()&&dataTarget!=brush.DataTarget()))throw std::invalid_argument("A stroke may edit only one target");
            const auto *target=map.DataLayer(brush.DataTarget());
            if(!target||target->locked)throw std::invalid_argument("Data layer is unavailable for painting");
            dataTarget=brush.DataTarget();
            const auto found=dataPending.find(key);const double before=target->cells.At(cell);
            const double value=brush.PaintData(cell,found==dataPending.end()?before:found->second.after);
            if(!std::isfinite(value)||value<target->minimum||value>target->maximum)throw std::invalid_argument("Brush returned an invalid data value");
            dataPending[key]={cell,before,value};return;
        }
        if(!dataTarget.empty())throw std::invalid_argument("A stroke may edit only one target");
        const auto found=pending.find(key);
        const auto before=map.Tile(layer,cell);
        const auto value=brush.Paint(cell,found==pending.end()?before:found->second.after);
        if(value && !map.Definition(value))throw std::invalid_argument("Brush returned an undefined tile");
        pending[key]={cell,before,value};
    }
    void Rectangle(GridCoordinate a,GridCoordinate b,const EnvironmentBrush &brush,bool filled=true) {
        const int left=std::min(a.column,b.column),right=std::max(a.column,b.column);
        const int top=std::min(a.row,b.row),bottom=std::max(a.row,b.row);
        for(int y=std::max(0,top);y<=std::min(map.Rows()-1,bottom);++y)
            for(int x=std::max(0,left);x<=std::min(map.Columns()-1,right);++x)
                if(filled || x==left || x==right || y==top || y==bottom)Sample({x,y},brush);
    }
    void Line(GridCoordinate a,GridCoordinate b,const EnvironmentBrush &brush,int radius=0) {
        // Clip before rasterizing, keeping out-of-bounds drags bounded in cost.
        double t0=0,t1=1,dx=double(b.column)-a.column,dy=double(b.row)-a.row;
        const auto clip=[&](double p,double q){
            if(p==0)return q>=0;
            const double t=q/p;
            if(p<0){if(t>t1)return false;t0=std::max(t0,t);}
            else {if(t<t0)return false;t1=std::min(t1,t);}return true;
        };
        if(!clip(-dx,a.column)||!clip(dx,double(map.Columns()-1)-a.column)||
           !clip(-dy,a.row)||!clip(dy,double(map.Rows()-1)-a.row))return;
        GridCoordinate end{int(std::lround(a.column+t1*dx)),int(std::lround(a.row+t1*dy))};
        a={int(std::lround(a.column+t0*dx)),int(std::lround(a.row+t0*dy))};
        const int xDelta=std::abs(end.column-a.column),yDelta=-std::abs(end.row-a.row);
        const int sx=a.column<end.column?1:-1,sy=a.row<end.row?1:-1;
        int error=xDelta+yDelta;
        for(;;){if(radius>0)Circle(a,radius,brush);else Sample(a,brush);if(a==end)break;const int twice=2*error;
            if(twice>=yDelta){error+=yDelta;a.column+=sx;}if(twice<=xDelta){error+=xDelta;a.row+=sy;}}
    }
    void Circle(GridCoordinate center,int radius,const EnvironmentBrush &brush) {
        if(radius<0)throw std::invalid_argument("Brush radius cannot be negative");
        const auto r=std::int64_t(radius);
        const int left=int(std::max<std::int64_t>(0,std::int64_t(center.column)-r));
        const int right=int(std::min<std::int64_t>(map.Columns()-1,std::int64_t(center.column)+r));
        const int top=int(std::max<std::int64_t>(0,std::int64_t(center.row)-r));
        const int bottom=int(std::min<std::int64_t>(map.Rows()-1,std::int64_t(center.row)+r));
        for(int y=top;y<=bottom;++y)for(int x=left;x<=right;++x){
            const double dx=double(x)-center.column,dy=double(y)-center.row;
            if(dx*dx+dy*dy<=double(radius)*radius)Sample({x,y},brush);
        }
    }
    void Fill(GridCoordinate seed,const EnvironmentBrush &brush) {
        if(!Contains(seed))return;
        const auto *data=brush.DataTarget().empty()?nullptr:map.DataLayer(brush.DataTarget());
        if(!brush.DataTarget().empty()&&!data)throw std::invalid_argument("Unknown brush data layer");
        const auto target=map.Tile(layer,seed);
        const double dataValue=data?data->cells.At(seed):0;
        std::vector<bool> visited(std::size_t(map.Columns())*map.Rows());
        std::queue<GridCoordinate> queue;queue.push(seed);
        while(!queue.empty()){
            const auto cell=queue.front();queue.pop();if(!Contains(cell))continue;
            const auto index=std::size_t(cell.row)*map.Columns()+cell.column;
            if(visited[index])continue;visited[index]=true;
            if(data?data->cells.At(cell)!=dataValue:map.Tile(layer,cell)!=target)continue;
            Sample(cell,brush);
            queue.push({cell.column-1,cell.row});queue.push({cell.column+1,cell.row});
            queue.push({cell.column,cell.row-1});queue.push({cell.column,cell.row+1});
        }
    }
    TilemapPatch Preview() const {
        TilemapPatch patch;patch.layer=layer;
        for(const auto &[key,change]:pending)if(change.before!=change.after)patch.changes.push_back(change);
        patch.dataTarget=dataTarget;
        for(const auto &[key,change]:dataPending)if(change.before!=change.after)patch.dataChanges.push_back(change);
        return patch;
    }
    bool Commit(Tilemap2D &target) const {
        return &target==&map && target.Revision()==revision && Preview().Apply(target);
    }
    void Cancel(){pending.clear();dataPending.clear();dataTarget.clear();}
private:
    const Tilemap2D &map;
    std::size_t layer;
    std::uint64_t revision;
    std::optional<Rectanglef> bounds;
    std::map<std::size_t,TileChange> pending;
    std::map<std::size_t,DataChange> dataPending;
    std::string dataTarget;
};
}
