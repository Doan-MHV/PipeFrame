#pragma once
#include <PipeFrame/Environment/Tilemap2D.h>

#include <algorithm>
#include <iomanip>
#include <istream>
#include <ostream>

namespace pipeframe {
class TilemapSerializer {
public:
    static bool Save(const Tilemap2D& map, std::ostream& output) {
        output << "PIPEFRAME_TILEMAP 3\n"
               << map.Columns() << ' ' << map.Rows() << ' ' << std::setprecision(9) << map.CellSize() << ' '
               << map.Origin().x << ' ' << map.Origin().y << '\n';
        output << std::quoted(map.Tileset()) << '\n';
        std::vector<TileId> ids;
        for (const auto& [id, tile] : map.Definitions())
            ids.push_back(id);
        std::sort(ids.begin(), ids.end());
        output << ids.size() << '\n';
        for (auto id : ids) {
            const auto& tile = *map.Definition(id);
            output << id << ' ' << unsigned(tile.tint.red) << ' ' << unsigned(tile.tint.green) << ' '
                   << unsigned(tile.tint.blue) << ' ' << unsigned(tile.tint.alpha) << ' ' << tile.solid << '\n';
        }
        output << map.Layers().size() << '\n';
        for (const auto& layer : map.Layers()) {
            output << std::quoted(layer.name) << ' ' << layer.visible << ' ' << layer.collision << ' ' << layer.locked
                   << '\n';
            for (int y = 0; y < map.Rows(); ++y) {
                for (int x = 0; x < map.Columns(); ++x)
                    output << layer.cells.At({x, y}) << ' ';
                output << '\n';
            }
        }
        output << map.DataLayers().size() << '\n' << std::setprecision(17);
        for (const auto& layer : map.DataLayers()) {
            output << std::quoted(layer.id) << ' ' << layer.minimum << ' ' << layer.maximum << ' ' << layer.locked
                   << '\n';
            for (int y = 0; y < map.Rows(); ++y) {
                for (int x = 0; x < map.Columns(); ++x)
                    output << layer.cells.At({x, y}) << ' ';
                output << '\n';
            }
        }
        return bool(output);
    }
    static std::optional<Tilemap2D> Load(std::istream& input, std::string& error) {
        error.clear();
        try {
            std::string header;
            unsigned version;
            int columns, rows;
            float size;
            Vector2f origin;
            if (!(input >> header >> version) || header != "PIPEFRAME_TILEMAP" || (version < 1 || version > 3) ||
                !(input >> columns >> rows >> size >> origin.x >> origin.y))
                throw std::invalid_argument("Invalid tilemap header");
            Tilemap2D map(columns, rows, size, origin);
            if (version >= 2) {
                std::string tileset;
                if (!(input >> std::quoted(tileset))) throw std::invalid_argument("Invalid tileset reference");
                map.SetTileset(std::move(tileset));
            }
            std::size_t count;
            if (!(input >> count) || count > 65536) throw std::invalid_argument("Invalid tile definition count");
            for (std::size_t i = 0; i < count; ++i) {
                TileId id;
                unsigned r, g, b, a;
                bool solid;
                if (!(input >> id >> r >> g >> b >> a >> solid) || r > 255 || g > 255 || b > 255 || a > 255 ||
                    map.Definition(id))
                    throw std::invalid_argument("Invalid tile definition");
                map.DefineTile({id, {std::uint8_t(r), std::uint8_t(g), std::uint8_t(b), std::uint8_t(a)}, solid});
            }
            if (!(input >> count) || count > 32) throw std::invalid_argument("Invalid layer count");
            for (std::size_t i = 0; i < count; ++i) {
                std::string name;
                bool visible, collision, locked;
                if (!(input >> std::quoted(name) >> visible >> collision >> locked))
                    throw std::invalid_argument("Invalid layer metadata");
                const auto layer = map.AddLayer(name);
                for (int y = 0; y < rows; ++y)
                    for (int x = 0; x < columns; ++x) {
                        TileId tile;
                        if (!(input >> tile) || !map.SetTile(layer, {x, y}, tile))
                            throw std::invalid_argument("Invalid tile cell");
                    }
                map.SetLayerFlags(layer, visible, collision, locked);
            }
            if (version >= 3) {
                if (!(input >> count) || count > 32) throw std::invalid_argument("Invalid data layer count");
                for (std::size_t i = 0; i < count; ++i) {
                    std::string id;
                    double minimum, maximum;
                    bool locked;
                    if (!(input >> std::quoted(id) >> minimum >> maximum >> locked))
                        throw std::invalid_argument("Invalid data layer metadata");
                    map.AddDataLayer(id, minimum, maximum);
                    for (int y = 0; y < rows; ++y)
                        for (int x = 0; x < columns; ++x) {
                            double value;
                            if (!(input >> value) || !map.SetData(id, {x, y}, value))
                                throw std::invalid_argument("Invalid data cell");
                        }
                    map.SetDataLayerLocked(id, locked);
                }
            }
            input >> std::ws;
            if (!input.eof()) throw std::invalid_argument("Unexpected trailing tilemap data");
            return map;
        } catch (const std::exception& failure) {
            error = failure.what();
            return std::nullopt;
        }
    }
};
}  // namespace pipeframe
