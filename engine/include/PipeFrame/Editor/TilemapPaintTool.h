#pragma once

#include <PipeFrame/Editor/EditorTool.h>
#include <PipeFrame/Environment/TilemapEdit.h>
#include <PipeFrame/Input/InputEvent.h>
#include <memory>
#include <optional>
#include <string>

namespace pipeframe {

enum class TilemapPaintShape { Pencil, Line, Rectangle, Outline, Fill, Circle, Eyedropper, Ruler, Selection };

// The document must outlive this tool. Destroy the tool before replacing its map.
// Hosts provide inverse-camera/transform cell coordinates, including outside cells
// during capture, and route captured events before other viewport interactions.
class TilemapPaintTool final : public EditorTool {
public:
    struct Result {
        bool consumed{};
        // Already applied once; append to history without applying it again.
        std::optional<TilemapPatch> committed;
    };

    explicit TilemapPaintTool(Tilemap2D &document) : document(document) {}
    ~TilemapPaintTool()override{SetEnabled(false);if(brush)brush->OnTeardown();}
    std::string_view GetToolId() const override { return "pipeframe.tilemap.paint"; }
    bool IsEnabled() const override { return enabled; }
    void SetEnabled(bool value) override {
        if(!value){Cancel();selection.reset();hover.reset();}
        if(enabled!=value&&brush){if(value)brush->OnActivate();else brush->OnDeactivate();}
        enabled=value;
    }
    void Configure(std::size_t targetLayer, std::shared_ptr<const EnvironmentBrush> effect,
                   TilemapPaintShape mode = TilemapPaintShape::Pencil) {
        Cancel(); if(brush){if(enabled)brush->OnDeactivate();if(brush.get()!=effect.get())brush->OnTeardown();} if(layer!=targetLayer)selection.reset(); sampledTile.reset(); layer = targetLayer; brush = std::move(effect); shape = mode;if(enabled&&brush)brush->OnActivate();
    }
    void SetBrushOptions(int value,bool constrain) {
        if(value<0||value>64)throw std::invalid_argument("Brush radius must be between 0 and 64 cells");
        Cancel();radius=value;axisConstraint=constrain;
    }
    void SetLayer(std::size_t value){Cancel();selection.reset();layer=value;}
    int BrushRadius()const{return radius;}
    bool AxisConstraint()const{return axisConstraint;}
    std::size_t Layer()const{return layer;}
    TilemapPaintShape Shape()const{return shape;}
    std::optional<GridCellRange> Selection()const {
        if(edit&&shape==TilemapPaintShape::Selection)return SelectionRange(anchor,endpoint);
        return selection;
    }
    void ClearSelection(){Cancel();selection.reset();}
    void ClearHover(){hover.reset();}
    std::optional<GridCoordinate> HoverCell()const{return HasPointerCapture()?std::optional<GridCoordinate>{endpoint}:hover;}
    std::optional<std::pair<GridCoordinate,GridCoordinate>> Gesture()const{return edit?std::optional{std::pair{anchor,endpoint}}:std::nullopt;}
    void SetBounds(std::optional<Rectanglef> value) { if(bounds!=value){Cancel();selection.reset();bounds=value;} }
    std::optional<TileId> SampledTile() const { return sampledTile; }
    std::optional<std::pair<GridCoordinate,GridCoordinate>> Measurement() const { return measurement; }
    bool HasPointerCapture() const { return bool(edit); }
    TilemapPatch Preview() const {
        return edit && document.Revision() == revision ? edit->Preview() : TilemapPatch{};
    }
    const std::string &LastError() const { return error; }
    void Cancel() { if(edit&&brush)brush->OnCancel();edit.reset(); strokeHorizontal.reset(); visited.clear(); fillPrepared = false; measurement.reset(); }

    // mayBegin is the host's viewport/UI hit-test result. Once captured, moves and
    // release remain routed here even when mayBegin is false (e.g. over a panel).
    // A missing coordinate on release cancels rather than committing a stale preview.
    Result HandleEvent(const InputEvent &event, std::optional<GridCoordinate> cell = {},
                       bool mayBegin = false) {
        if (!enabled) return {};
        if(event.type==InputEventType::KeyPressed&&!HasPointerCapture()){
            const auto *key=event.GetIf<KeyInput>();
            if(key&&key->key==InputKey::L&&!key->control&&!key->system&&!key->alt){axisConstraint=!axisConstraint;return {true,{}};}
        }
        if(event.type==InputEventType::PointerMoved)hover=mayBegin?cell:std::nullopt;
        if(event.type==InputEventType::KeyPressed&&event.GetIf<KeyInput>()&&event.GetIf<KeyInput>()->key==InputKey::Escape&&!HasPointerCapture()){
            const bool had=bool(selection)||bool(measurement);ClearSelection();return {had,{}};
        }
        const bool captured = HasPointerCapture();
        if (captured && document.Revision() != revision) {
            error = "Tilemap changed during the stroke"; Cancel(); return {true, {}};
        }
        if (captured && ((event.type == InputEventType::KeyPressed &&
                          event.GetIf<KeyInput>() && event.GetIf<KeyInput>()->key == InputKey::Escape) ||
                         (event.type == InputEventType::FocusChanged &&
                          event.GetIf<FocusInput>() && !event.GetIf<FocusInput>()->focused) ||
                         (event.type == InputEventType::PointerPresenceChanged &&
                          event.GetIf<PointerPresenceInput>() && !event.GetIf<PointerPresenceInput>()->inside))) {
            Cancel(); return {true, {}};
        }
        try {
            if (event.type == InputEventType::PointerPressed) {
                const auto *pointer = event.GetIf<PointerInput>();
                if (!pointer || pointer->button != PointerButton::Left) return {};
                if (captured) return {true, {}};
                if (!mayBegin || !cell || !document.Contains(*cell) || !brush) return {};
                error.clear();
                if(brush->DataTarget().empty()&&layer<document.Layers().size()&&!document.Layers()[layer].visible&&shape!=TilemapPaintShape::Ruler&&shape!=TilemapPaintShape::Selection&&shape!=TilemapPaintShape::Eyedropper){error="Show the layer before painting";return {true,{}};}
                edit = std::make_unique<TilemapEdit>(document, layer, bounds,!brush->DataTarget().empty()||shape==TilemapPaintShape::Ruler||shape==TilemapPaintShape::Selection||shape==TilemapPaintShape::Eyedropper);
                if (!edit->Contains(*cell)) { Cancel(); return {}; }
                hover=*cell;
                revision = document.Revision();strokeHorizontal.reset(); anchor = previous = *cell;
                brush->OnBegin(*cell);Update(*cell);NotifyPreview(); return {true, {}};
            }
            if (!captured) return {};
            if (event.type == InputEventType::PointerMoved && event.GetIf<PointerMoveInput>()) {
                if (cell) {Update(*cell);NotifyPreview();}
                return {true, {}};
            }
            if (event.type == InputEventType::PointerReleased) {
                const auto *pointer = event.GetIf<PointerInput>();
                if (!pointer || pointer->button != PointerButton::Left) return {};
                if (!cell) { Cancel(); return {true, {}}; }
                Update(*cell);
                if(shape==TilemapPaintShape::Selection){selection=SelectionRange(anchor,endpoint);brush->OnEnd();edit.reset();visited.clear();return {true,{}};}
                if(shape==TilemapPaintShape::Ruler){brush->OnEnd();edit.reset();visited.clear();return {true,{}};}
                auto patch = edit->Preview();
                if (patch.Empty()) { brush->OnEnd();edit.reset();Cancel(); return {true, {}}; }
                if (!edit->Commit(document)) {
                    error = "Tilemap changed during the stroke"; Cancel(); return {true, {}};
                }
                brush->OnEnd();edit.reset();Cancel(); return {true, std::move(patch)};
            }
        } catch (const std::exception &exception) {
            error = exception.what(); Cancel(); return {true, {}};
        }
        return {};
    }

private:
    std::optional<GridCellRange> SelectionRange(GridCoordinate a,GridCoordinate b)const {
        int left=std::max(0,std::min(a.column,b.column)),right=std::min(document.Columns()-1,std::max(a.column,b.column));
        int top=std::max(0,std::min(a.row,b.row)),bottom=std::min(document.Rows()-1,std::max(a.row,b.row));
        if(bounds){
            left=std::max(left,int(std::floor((bounds->position.x-document.Origin().x)/document.CellSize())));
            top=std::max(top,int(std::floor((bounds->position.y-document.Origin().y)/document.CellSize())));
            right=std::min(right,int(std::ceil((bounds->position.x+bounds->size.x-document.Origin().x)/document.CellSize()))-1);
            bottom=std::min(bottom,int(std::ceil((bounds->position.y+bounds->size.y-document.Origin().y)/document.CellSize()))-1);
        }
        if(left>right||top>bottom)return {};
        return GridCellRange{{left,top},{right,bottom}};
    }
    void NotifyPreview(){if(edit&&brush){const auto patch=edit->Preview();brush->OnPreview(patch.changes.size()+patch.dataChanges.size());}}
    void Update(GridCoordinate cell) {
        brush->OnUpdate(cell);
        if(axisConstraint){
            if(!strokeHorizontal&&cell!=anchor)strokeHorizontal=std::abs(cell.column-anchor.column)>=std::abs(cell.row-anchor.row);
            if(strokeHorizontal){if(*strokeHorizontal)cell.row=anchor.row;else cell.column=anchor.column;}
        }
        endpoint=cell;
        if (shape == TilemapPaintShape::Pencil) {
            // A custom effect runs once per cell per stroke, independent of input
            // frequency, overlapping segments, or the final release event.
            class Once final : public EnvironmentBrush {
            public:
                Once(const EnvironmentBrush &effect, std::set<std::pair<int,int>> &seen)
                    : effect(effect), seen(seen) {}
                TileId Paint(GridCoordinate at, TileId old) const override {
                    return seen.emplace(at.column, at.row).second ? effect.Paint(at, old) : old;
                }
                std::string_view DataTarget()const override{return effect.DataTarget();}
                double PaintData(GridCoordinate at,double old)const override{
                    return seen.emplace(at.column,at.row).second?effect.PaintData(at,old):old;
                }
            private:
                const EnvironmentBrush &effect;
                std::set<std::pair<int,int>> &seen;
            } once(*brush, visited);
            edit->Line(previous, cell, once,radius); previous = cell;
            return;
        }
        if (shape == TilemapPaintShape::Fill && fillPrepared) return;
        // Shape endpoints replace the preview; they do not accumulate rectangles.
        edit->Cancel();
        switch (shape) {
        case TilemapPaintShape::Line: edit->Line(anchor, cell, *brush); break;
        case TilemapPaintShape::Rectangle: edit->Rectangle(anchor, cell, *brush); break;
        case TilemapPaintShape::Outline: edit->Rectangle(anchor, cell, *brush, false); break;
        case TilemapPaintShape::Fill: edit->Fill(anchor, *brush); fillPrepared = true; break;
        case TilemapPaintShape::Circle: {
            const auto dx=double(cell.column)-anchor.column,dy=double(cell.row)-anchor.row;
            edit->Circle(anchor,int(std::min(1000000.0,std::hypot(dx,dy))),*brush); break;
        }
        case TilemapPaintShape::Eyedropper: sampledTile=document.Tile(layer,anchor); break;
        case TilemapPaintShape::Ruler: measurement=std::pair{anchor,cell}; break;
        case TilemapPaintShape::Selection: break;
        case TilemapPaintShape::Pencil: break;
        }
    }

    std::optional<GridCellRange> selection;
    std::optional<GridCoordinate> hover;
    int radius{};bool axisConstraint{};
    std::optional<bool> strokeHorizontal;
    GridCoordinate endpoint{};
    Tilemap2D &document;
    std::optional<Rectanglef> bounds;
    std::size_t layer{};
    std::shared_ptr<const EnvironmentBrush> brush;
    TilemapPaintShape shape{TilemapPaintShape::Pencil};
    bool enabled{};
    bool fillPrepared{};
    std::uint64_t revision{};
    GridCoordinate anchor{}, previous{};
    std::unique_ptr<TilemapEdit> edit;
    std::set<std::pair<int,int>> visited;
    std::string error;
    std::optional<TileId> sampledTile;
    std::optional<std::pair<GridCoordinate,GridCoordinate>> measurement;
};
}
