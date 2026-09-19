#pragma once

#include <PipeFrame/Editor/BrushTool.h>
#include <PipeFrame/Editor/TilemapPaintTool.h>
#include <PipeFrame/Editor/VisualAssetEditor.h>
#include <PipeFrame/Environment/TilemapSerializer.h>
#include <PipeFrame/Environment/VisualAssets2D.h>
#include <PipeFrame/Project/AssetDatabase.h>

#include <fstream>
#include <sstream>
#include <variant>

namespace pipeframe {

// Owns the authoring map and its gesture tool. Runtime asset caches remain immutable.
// The host owns the database and must close this editor before changing projects.
class TilemapAssetEditor final : public EditorTool {
public:
    std::string_view GetToolId() const override { return "pipeframe.tilemap.asset-editor"; }
    bool IsEnabled() const override { return tool && tool->IsEnabled(); }
    void SetEnabled(bool enabled) override {
        if (tool) tool->SetEnabled(enabled);
    }
    void SetBounds(std::optional<Rectanglef> bounds) {
        paintBounds = bounds;
        if (tool) tool->SetBounds(bounds);
    }
    bool IsDirty() const { return !savedCursor || *savedCursor != cursor; }
    const Tilemap2D* Document() const { return map.get(); }
    const assets::AssetId& AssetId() const { return assetId; }
    const std::string& LastError() const { return error; }
    bool HasPointerCapture() const { return tool && tool->HasPointerCapture(); }
    std::optional<TileId> SampledTile() const { return tool ? tool->SampledTile() : std::nullopt; }
    std::optional<std::pair<GridCoordinate, GridCoordinate>> Measurement() const {
        return tool ? tool->Measurement() : std::nullopt;
    }
    TilemapPatch Preview() const { return tool ? tool->Preview() : TilemapPatch{}; }

    const auto& Brushes() const { return brushes; }
    const BrushTool* ActiveBrush() const { return activeBrush.get(); }
    void ClearBrushes() {
        // Drop all plugin-owned functions/objects before unloading its library.
        if (tool) {
            tool->SetEnabled(false);
            tool->Configure(tool->Layer(), std::make_shared<TileBrush>(0), tool->Shape());
        }
        activeBrush.reset();
        brushes.clear();
    }
    void SetBrushes(std::vector<BrushToolDescriptor> value) {
        ClearBrushes();
        brushes = std::move(value);
    }
    bool SelectBrush(const std::string& id) {
        if (!map || !tool) return Fail("Open a map first");
        tool->SetEnabled(false);
        if (id.empty()) {
            activeBrush.reset();
            tool->Configure(tool->Layer(), std::make_shared<TileBrush>(0), tool->Shape());
            return true;
        }
        for (const auto& descriptor : brushes)
            if (descriptor.id == id) {
                std::shared_ptr<BrushTool> next;
                try {
                    next = descriptor.create();
                } catch (const std::exception& e) {
                    return Fail(e.what());
                } catch (...) {
                    return Fail("Brush factory failed");
                }
                if (!next) return Fail("Brush factory returned no brush");
                if (!next->DataTarget().empty() && !map->DataLayer(next->DataTarget())) {
                    try {
                        auto initialized = *map;
                        initialized.AddDataLayer(std::string(next->DataTarget()), next->DataMinimum(),
                                                 next->DataMaximum());
                        for (int y = 0; y < map->Rows(); ++y)
                            for (int x = 0; x < map->Columns(); ++x)
                                if (!initialized.SetData(next->DataTarget(), {x, y}, next->InitialData(*map, {x, y})))
                                    return Fail("Invalid brush layer initialization");
                        if (!Replace(std::move(initialized))) return false;
                    } catch (const std::exception& e) {
                        return Fail(e.what());
                    }
                }
                activeBrush = std::move(next);
                tool->Configure(tool->Layer(), activeBrush, tool->Shape());
                tool->SetEnabled(true);
                error.clear();
                return true;
            }
        return Fail("Unknown project brush");
    }
    bool SetBrushSetting(const std::string& key, const PropertyValue& value) {
        if (!activeBrush || !tool) return Fail("Select a brush first");
        tool->Cancel();
        return activeBrush->SetSetting(key, value, error);
    }
    void ConfigureGesture(std::size_t layer, TileId tile, TilemapPaintShape shape) {
        std::shared_ptr<const EnvironmentBrush> effect = activeBrush;
        if (!effect)
            effect = std::make_shared<TileBrush>(tile);
        else if (tile == 0 && !activeBrush->DataTarget().empty())
            effect = std::make_shared<EraseDataBrush>(std::string(activeBrush->DataTarget()));
        Configure(layer, std::move(effect), shape);
    }
    bool Open(assets::AssetDatabase& assets, const assets::AssetId& id) {
        error.clear();
        if (IsDirty() || HasPointerCapture())
            return Fail("Save or discard the active tilemap before opening another asset");
        const auto* record = assets.Find(id);
        if (!record || record->type != assets::AssetType::Tilemap) return Fail("Select a tilemap asset");
        const auto path = assets.GetProjectDirectory() / record->sourcePath;
        std::string contents;
        if (!Read(path, contents)) return Fail("Cannot read tilemap source");
        std::istringstream input(contents);
        auto loaded = TilemapSerializer::Load(input, error);
        if (!loaded) return false;
        if (loaded->Layers().empty()) return Fail("The map needs at least one layer before editing");
        // Reset borrowed map references before replacing the owned document.
        Close(true);
        database = &assets;
        projectRoot = assets.GetProjectDirectory();
        assetId = id;
        sourcePath = record->sourcePath;
        baseline = std::move(contents);
        map = std::make_unique<Tilemap2D>(std::move(*loaded));
        tool = std::make_unique<TilemapPaintTool>(*map);
        return true;
    }
    bool Close(bool discard = false) {
        if (!discard && (IsDirty() || HasPointerCapture())) return Fail("Tilemap has unsaved work");
        ClearBrushes();
        tool.reset();
        map.reset();
        paintBounds.reset();
        resizePlan.reset();
        resizeSummary.clear();
        history.clear();
        cursor = 0;
        savedCursor = 0;
        database = nullptr;
        projectRoot.clear();
        sourcePath.clear();
        assetId.clear();
        baseline.clear();
        return true;
    }
    void Configure(std::size_t layer, std::shared_ptr<const EnvironmentBrush> brush,
                   TilemapPaintShape shape = TilemapPaintShape::Pencil) {
        if (tool) tool->Configure(layer, std::move(brush), shape);
    }
    bool AddDataLayer(std::string id, double minimum, double maximum) {
        if (!map || HasPointerCapture()) return Fail("Finish the stroke before adding a data layer");
        try {
            auto next = *map;
            next.AddDataLayer(std::move(id), minimum, maximum);
            return Replace(std::move(next));
        } catch (const std::exception& e) {
            return Fail(e.what());
        }
    }
    bool HandleEvent(const InputEvent& event, std::optional<GridCoordinate> cell = {}, bool mayBegin = false) {
        if (!tool) return false;
        auto result = tool->HandleEvent(event, cell, mayBegin);
        if (result.committed) {
            if (savedCursor && *savedCursor > cursor) savedCursor.reset();
            history.erase(history.begin() + cursor, history.end());
            history.push_back(std::move(*result.committed));
            ++cursor;
        }
        error = tool->LastError();
        return result.consumed;
    }
    void SetBrushOptions(int radius, bool axis) {
        if (tool) try {
                tool->SetBrushOptions(radius, axis);
                error.clear();
            } catch (const std::exception& e) {
                Fail(e.what());
            }
    }
    int BrushRadius() const { return tool ? tool->BrushRadius() : 0; }
    bool AxisConstraint() const { return tool && tool->AxisConstraint(); }
    std::size_t ActiveLayer() const { return tool ? tool->Layer() : 0; }
    TilemapPaintShape Shape() const { return tool ? tool->Shape() : TilemapPaintShape::Pencil; }
    std::optional<GridCellRange> Selection() const { return tool ? tool->Selection() : std::nullopt; }
    void ClearHover() {
        if (tool) tool->ClearHover();
    }
    std::optional<GridCoordinate> HoverCell() const { return tool ? tool->HoverCell() : std::nullopt; }
    auto Gesture() const { return tool ? tool->Gesture() : std::optional<std::pair<GridCoordinate, GridCoordinate>>{}; }
    bool EditLayer(std::string action, std::size_t index, std::string name = {}) {
        if (!map || !tool) return Fail("Open a map first");
        try {
            auto next = *map;
            if (action == "add")
                next.AddLayer(name.empty() ? "Layer " + std::to_string(next.Layers().size() + 1) : name);
            else if (action == "rename")
                next.RenameLayer(index, std::move(name));
            else if (action == "remove")
                next.RemoveLayer(index);
            else if (action == "up")
                next.MoveLayer(index, index + 1);
            else if (action == "down") {
                if (index == 0) return Fail("Layer is already at the bottom");
                next.MoveLayer(index, index - 1);
            } else {
                const auto layer = next.Layers().at(index);
                if (action != "visible" && action != "collision" && action != "lock")
                    return Fail("Unknown layer operation");
                next.SetLayerFlags(index, action == "visible" ? !layer.visible : layer.visible,
                                   action == "collision" ? !layer.collision : layer.collision,
                                   action == "lock" ? !layer.locked : layer.locked);
            }
            auto active = tool->Layer();
            if (action == "add")
                active = next.Layers().size() - 1;
            else if (action == "remove" && index < active)
                --active;
            else if (action == "up") {
                if (active == index)
                    ++active;
                else if (active == index + 1)
                    --active;
            } else if (action == "down") {
                if (active == index)
                    --active;
                else if (active + 1 == index)
                    ++active;
            }
            tool->SetLayer(std::min(active, next.Layers().size() - 1));
            return Replace(std::move(next));
        } catch (const std::exception& e) {
            return Fail(e.what());
        }
    }
    bool DefineTile(TileId id, Color tint, bool solid) {
        if (!map || id == 0) return Fail("Tile IDs start at 1");
        if (!map->Tileset().empty() && !map->Definition(id))
            return Fail("Atlas tile definitions come from its tileset");
        auto next = *map;
        next.DefineTile({id, tint, solid});
        return Replace(std::move(next));
    }
    bool PaintSelection(TileId tile, bool boundary = false) {
        if (!map || !tool || HasPointerCapture()) return Fail("Finish the selection first");
        try {
            if (!activeBrush && !map->Layers().at(tool->Layer()).visible) return Fail("Show the layer before painting");
            TilemapEdit edit(*map, tool->Layer(), paintBounds, bool(activeBrush));
            const TileBrush tileBrush(tile);
            class ClearData final : public EnvironmentBrush {
            public:
                explicit ClearData(std::string_view id) : id(id) {}
                std::string_view DataTarget() const override { return id; }
                double PaintData(GridCoordinate, double) const override { return 0; }

            private:
                std::string id;
            };
            const ClearData clear(activeBrush ? activeBrush->DataTarget() : std::string_view{});
            const EnvironmentBrush& brush =
                activeBrush ? (tile == 0 ? static_cast<const EnvironmentBrush&>(clear) : *activeBrush)
                            : static_cast<const EnvironmentBrush&>(tileBrush);
            auto range = tool->Selection();
            if (boundary) {
                auto bounds = paintBounds.value_or(map->Bounds());
                const auto origin = map->Origin();
                const auto size = map->CellSize();
                range = GridCellRange{
                    {std::max(0, int(std::floor((bounds.position.x - origin.x) / size))),
                     std::max(0, int(std::floor((bounds.position.y - origin.y) / size)))},
                    {std::min(map->Columns() - 1,
                              int(std::ceil((bounds.position.x + bounds.size.x - origin.x) / size)) - 1),
                     std::min(map->Rows() - 1,
                              int(std::ceil((bounds.position.y + bounds.size.y - origin.y) / size)) - 1)}};
            }
            if (!range) return Fail("Drag a selection first");
            edit.Rectangle(range->minimum, range->maximum, brush, !boundary);
            auto patch = edit.Preview();
            if (patch.Empty()) return true;
            if (!edit.Commit(*map)) return Fail("Map changed during selection edit");
            if (savedCursor && *savedCursor > cursor) savedCursor.reset();
            history.resize(cursor);
            history.emplace_back(std::move(patch));
            ++cursor;
            error.clear();
            return true;
        } catch (const std::exception& e) {
            return Fail(e.what());
        }
    }
    bool CanUndo() const { return cursor > 0; }
    bool CanRedo() const { return cursor < history.size(); }
    bool Undo() {
        if (!tool) return false;
        tool->Cancel();
        if (!CanUndo() || !ApplyHistory(cursor - 1, false)) return false;
        --cursor;
        return true;
    }
    bool Redo() {
        if (!tool) return false;
        tool->Cancel();
        if (!CanRedo() || !ApplyHistory(cursor, true)) return false;
        ++cursor;
        return true;
    }
    bool AssignTileset(const std::string& assetId) {
        if (!map || HasPointerCapture()) return Fail("Finish the stroke before changing tileset");
        const auto* record = database->Find(assetId);
        if (!record || record->type != assets::AssetType::Tileset || record->state != assets::AssetState::Ready)
            return Fail("Choose a ready tileset");
        VisualAssetEditor validation;
        if (!validation.Open(*database, assetId) || !validation.ValidateReferences())
            return Fail(validation.LastError());
        std::ifstream input(projectRoot / record->sourcePath);
        auto atlas = LoadVisualAsset<Tileset2D>(input, error);
        if (!atlas) return false;
        auto next = *map;
        const auto count = atlas->columns * atlas->rows;
        for (const auto& [id, tile] : next.Definitions())
            if (id > count) return Fail("Tileset has fewer tiles than the map definitions");
        next.SetTileset(assetId);
        for (std::uint32_t id = 1; id <= count; ++id)
            if (!next.Definition(id)) next.DefineTile({id, {255, 255, 255, 255}, false});
        return Replace(std::move(next));
    }
    bool PrepareResize(int columns, int rows, float cellSize, bool resample = false) {
        resizePlan.reset();
        resizeSummary.clear();
        if (!map || HasPointerCapture()) return Fail("Finish the stroke before resizing");
        try {
            if (resample) {
                if (columns <= 0 || rows <= 0) return Fail("Resolution must be positive");
                cellSize = map->Bounds().size.x / columns;
                if (std::abs(cellSize * rows - map->Bounds().size.y) > 0.001f)
                    return Fail("Keep the map aspect ratio for square-cell resampling");
            }
            Tilemap2D next(columns, rows, cellSize, map->Origin());
            next.SetTileset(map->Tileset());
            for (const auto& [id, tile] : map->Definitions())
                next.DefineTile(tile);
            std::size_t dropped = 0;
            for (std::size_t i = 0; i < map->Layers().size(); ++i) {
                const auto& layer = map->Layers()[i];
                next.AddLayer(layer.name);
                for (int y = 0; y < rows; ++y)
                    for (int x = 0; x < columns; ++x) {
                        const GridCoordinate old =
                            resample
                                ? GridCoordinate{std::min(map->Columns() - 1, int((x + .5) * map->Columns() / columns)),
                                                 std::min(map->Rows() - 1, int((y + .5) * map->Rows() / rows))}
                                : GridCoordinate{x, y};
                        if (map->Contains(old)) next.SetTile(i, {x, y}, map->Tile(i, old));
                    }
                if (!resample)
                    for (int y = 0; y < map->Rows(); ++y)
                        for (int x = 0; x < map->Columns(); ++x)
                            if ((x >= columns || y >= rows) && map->Tile(i, {x, y})) ++dropped;
                next.SetLayerFlags(i, layer.visible, layer.collision, layer.locked);
            }
            for (const auto& layer : map->DataLayers()) {
                next.AddDataLayer(layer.id, layer.minimum, layer.maximum);
                for (int y = 0; y < rows; ++y)
                    for (int x = 0; x < columns; ++x) {
                        const GridCoordinate old =
                            resample
                                ? GridCoordinate{std::min(map->Columns() - 1, int((x + .5) * map->Columns() / columns)),
                                                 std::min(map->Rows() - 1, int((y + .5) * map->Rows() / rows))}
                                : GridCoordinate{x, y};
                        if (map->Contains(old)) next.SetData(layer.id, {x, y}, layer.cells.At(old));
                    }
                next.SetDataLayerLocked(layer.id, layer.locked);
            }
            resizeRevision = map->Revision();
            resizePlan = std::move(next);
            resizeSummary = std::to_string(columns) + " x " + std::to_string(rows) + "; " +
                            (resample ? "resample same area" : std::to_string(dropped) + " painted cells cropped");
            error.clear();
            return true;
        } catch (const std::exception& failure) {
            resizePlan.reset();
            return Fail(failure.what());
        }
    }
    std::optional<Rectanglef> ResizeBounds() const {
        return resizePlan ? std::optional{resizePlan->Bounds()} : std::nullopt;
    }
    const std::string& ResizeSummary() const { return resizeSummary; }
    bool ApplyResize() {
        if (!resizePlan || !map || map->Revision() != resizeRevision || HasPointerCapture())
            return Fail("Preview the current map before applying resize");
        auto next = std::move(*resizePlan);
        resizePlan.reset();
        resizeSummary.clear();
        return Replace(std::move(next));
    }
    // Copy the saved authoring document without changing this document or its history.
    // Assignment to a scene object belongs to the host's existing scene undo stack.
    std::optional<assets::AssetId> CopyAsset() {
        error.clear();
        if (!map || !database || database->GetProjectDirectory() != projectRoot) {
            Fail("Open a map in the active project first");
            return {};
        }
        if (database->HasActiveImport()) {
            Fail("Wait for the active import before copying");
            return {};
        }
        if (IsDirty() || HasPointerCapture()) {
            Fail("Save the map and finish the stroke before making it unique");
            return {};
        }
        std::string contents;
        const auto* record = database->Find(assetId);
        if (!record || record->sourcePath != sourcePath || !Read(projectRoot / sourcePath, contents) ||
            contents != baseline) {
            Fail("Map source changed; reopen it before making a copy");
            return {};
        }
        const auto folder = projectRoot / "Assets/Tilemaps";
        std::error_code ec;
        std::filesystem::create_directories(folder, ec);
        if (ec) {
            Fail("Cannot create map asset folder");
            return {};
        }
        auto destination = folder / (sourcePath.stem().string() + " Copy.pftilemap");
        for (int suffix = 2; std::filesystem::exists(destination); ++suffix)
            destination = folder / (sourcePath.stem().string() + " Copy " + std::to_string(suffix) + ".pftilemap");
        {
            std::ofstream output(destination, std::ios::binary);
            output << contents;
            output.close();
            if (!output) {
                std::filesystem::remove(destination, ec);
                Fail("Cannot write map copy");
                return {};
            }
        }
        auto copied = database->ImportNow({destination}, &error);
        if (!copied) std::filesystem::remove(destination, ec);
        return copied;
    }
    bool Save() {
        error.clear();
        if (!database || !map) return Fail("No tilemap is open");
        if (database->HasActiveImport()) return Fail("Wait for the active import before saving");
        if (HasPointerCapture()) return Fail("Finish or cancel the stroke before saving");
        if (database->GetProjectDirectory() != projectRoot) return Fail("Active project changed");
        const auto* record = database->Find(assetId);
        if (!record || record->sourcePath != sourcePath) return Fail("Tilemap asset moved or was removed");
        const auto path = projectRoot / sourcePath;
        std::string current;
        if (!Read(path, current) || current != baseline)
            return Fail("Tilemap source changed outside this editor; reopen it before saving");
        if (!IsDirty()) return true;
        std::ostringstream output;
        if (!TilemapSerializer::Save(*map, output)) return Fail("Cannot serialize tilemap");
        // A same-directory temporary file allows atomic source replacement.
        const auto temporary = path.string() + ".pipeframe-saving";
        std::error_code ec;
        if (std::filesystem::exists(temporary, ec) || ec) return Fail("Tilemap save temporary file is already present");
        {
            std::ofstream file(temporary, std::ios::binary);
            file << output.str();
            file.close();
            if (!file) {
                std::filesystem::remove(temporary, ec);
                return Fail("Cannot write tilemap source");
            }
        }
        std::filesystem::rename(temporary, path, ec);
        if (ec) {
            std::filesystem::remove(temporary, ec);
            return Fail("Cannot replace tilemap source");
        }
        baseline = output.str();
        savedCursor.reset();
        // Preserve dirty state if import fails, permitting a retry without losing edits.
        if (!database->Reimport(assetId, &error)) return false;
        savedCursor = cursor;
        return true;
    }

private:
    struct Snapshot {
        Tilemap2D before, after;
    };
    bool Replace(Tilemap2D next) {
        tool->ClearSelection();
        if (savedCursor && *savedCursor > cursor) savedCursor.reset();
        history.erase(history.begin() + cursor, history.end());
        history.emplace_back(Snapshot{*map, std::move(next)});
        map->ReplaceWith(std::get<Snapshot>(history.back()).after);
        ++cursor;
        error.clear();
        return true;
    }
    bool ApplyHistory(std::size_t index, bool forward) {
        return std::visit(
            [&](const auto& entry) {
                using T = std::decay_t<decltype(entry)>;
                if constexpr (std::is_same_v<T, TilemapPatch>)
                    return entry.Apply(*map, forward);
                else {
                    map->ReplaceWith(forward ? entry.after : entry.before);
                    tool->SetLayer(std::min(tool->Layer(), map->Layers().size() - 1));
                    return true;
                }
            },
            history[index]);
    }
    static bool Read(const std::filesystem::path& path, std::string& contents) {
        std::ifstream input(path, std::ios::binary);
        if (!input) return false;
        std::ostringstream buffer;
        buffer << input.rdbuf();
        if (input.bad()) return false;
        contents = buffer.str();
        return true;
    }
    bool Fail(std::string message) {
        error = std::move(message);
        return false;
    }
    std::optional<Rectanglef> paintBounds;
    assets::AssetDatabase* database{};
    std::filesystem::path projectRoot, sourcePath;
    assets::AssetId assetId;
    std::string baseline, error;
    std::unique_ptr<Tilemap2D> map;
    std::unique_ptr<TilemapPaintTool> tool;  // Destroyed before its referenced map.
    std::vector<BrushToolDescriptor> brushes;
    std::shared_ptr<BrushTool> activeBrush;
    std::vector<std::variant<TilemapPatch, Snapshot>> history;
    std::optional<Tilemap2D> resizePlan;
    std::uint64_t resizeRevision{};
    std::string resizeSummary;
    std::size_t cursor{};
    std::optional<std::size_t> savedCursor{0};
};
}  // namespace pipeframe
