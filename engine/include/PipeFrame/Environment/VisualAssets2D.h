#pragma once
#include <PipeFrame/Project/ComponentSchema.h>

#include <iomanip>
#include <istream>
#include <ostream>

namespace pipeframe {
struct Material2D {
    AssetReference texture;
    Color tint{255, 255, 255, 255};
    Vector2f uvScale{1, 1};
    bool smooth{false};
    static auto Schema() {
        return ComponentSchema<Material2D>("pipeframe.material2d", "2D Material")
            .Editable({.key = "texture",
                       .displayName = "Texture",
                       .kind = PropertyKind::AssetReference,
                       .defaultValue = AssetReference{},
                       .editorHint = "asset:Texture"},
                      &Material2D::texture)
            .Editable({.key = "tint",
                       .displayName = "Tint",
                       .kind = PropertyKind::Color,
                       .defaultValue = Color{255, 255, 255, 255}},
                      &Material2D::tint)
            .Editable({.key = "uvScale",
                       .displayName = "Ground UV Repeats",
                       .kind = PropertyKind::Vector2,
                       .defaultValue = Vector2f{1, 1}},
                      &Material2D::uvScale)
            .Editable({.key = "smooth",
                       .displayName = "Smooth Filtering",
                       .kind = PropertyKind::Boolean,
                       .defaultValue = false},
                      &Material2D::smooth)
            .Validate("UV repeats must be positive, finite and at most 4096", [](const auto& v) {
                return std::isfinite(v.uvScale.x) && std::isfinite(v.uvScale.y) && v.uvScale.x > 0 && v.uvScale.y > 0 &&
                       v.uvScale.x <= 4096 && v.uvScale.y <= 4096;
            });
    }
};
struct Tileset2D {
    bool operator==(const Tileset2D&) const = default;
    AssetReference material;
    std::int64_t tileWidth{16}, tileHeight{16}, columns{1}, rows{1}, margin{}, spacing{};
    static auto Schema() {
        using K = PropertyKind;
        return ComponentSchema<Tileset2D>("pipeframe.tileset2d", "Tileset Atlas")
            .Editable({.key = "material",
                       .displayName = "Material",
                       .kind = K::AssetReference,
                       .defaultValue = AssetReference{},
                       .editorHint = "asset:Material"},
                      &Tileset2D::material)
            .Editable({.key = "tileWidth",
                       .displayName = "Tile Width",
                       .kind = K::Integer,
                       .defaultValue = std::int64_t{16},
                       .unit = "pixels",
                       .minimum = 1,
                       .maximum = 16384},
                      &Tileset2D::tileWidth)
            .Editable({.key = "tileHeight",
                       .displayName = "Tile Height",
                       .kind = K::Integer,
                       .defaultValue = std::int64_t{16},
                       .unit = "pixels",
                       .minimum = 1,
                       .maximum = 16384},
                      &Tileset2D::tileHeight)
            .Editable({.key = "columns",
                       .displayName = "Atlas Columns",
                       .kind = K::Integer,
                       .defaultValue = std::int64_t{1},
                       .minimum = 1,
                       .maximum = 256},
                      &Tileset2D::columns)
            .Editable({.key = "rows",
                       .displayName = "Atlas Rows",
                       .kind = K::Integer,
                       .defaultValue = std::int64_t{1},
                       .minimum = 1,
                       .maximum = 256},
                      &Tileset2D::rows)
            .Editable({.key = "margin",
                       .displayName = "Outer Margin",
                       .kind = K::Integer,
                       .defaultValue = std::int64_t{0},
                       .unit = "pixels",
                       .minimum = 0,
                       .maximum = 16384},
                      &Tileset2D::margin)
            .Editable({.key = "spacing",
                       .displayName = "Tile Spacing",
                       .kind = K::Integer,
                       .defaultValue = std::int64_t{0},
                       .unit = "pixels",
                       .minimum = 0,
                       .maximum = 16384},
                      &Tileset2D::spacing);
    }
    bool Fits(Vector2u image) const {
        return 2 * margin + columns * tileWidth + (columns - 1) * spacing <= image.x &&
               2 * margin + rows * tileHeight + (rows - 1) * spacing <= image.y;
    }
    std::optional<Rectanglef> Region(std::uint32_t id) const {
        if (id == 0 || id > columns * rows) return {};
        const auto index = id - 1;
        return Rectanglef{{float(margin + (index % columns) * (tileWidth + spacing)),
                           float(margin + (index / columns) * (tileHeight + spacing))},
                          {float(tileWidth), float(tileHeight)}};
    }
};
// Versioned, strict, deterministic authoring formats. Unknown versions never fall back.
inline bool SaveVisualAsset(const Material2D& value, std::ostream& out) {
    out << "PIPEFRAME_MATERIAL2D 1\n"
        << std::quoted(value.texture.assetId) << '\n'
        << unsigned(value.tint.r) << ' ' << unsigned(value.tint.g) << ' ' << unsigned(value.tint.b) << ' '
        << unsigned(value.tint.a) << '\n'
        << std::setprecision(9) << value.uvScale.x << ' ' << value.uvScale.y << ' ' << value.smooth << '\n';
    return bool(out);
}
inline bool SaveVisualAsset(const Tileset2D& value, std::ostream& out) {
    out << "PIPEFRAME_TILESET2D 1\n"
        << std::quoted(value.material.assetId) << '\n'
        << value.tileWidth << ' ' << value.tileHeight << ' ' << value.columns << ' ' << value.rows << ' '
        << value.margin << ' ' << value.spacing << '\n';
    return bool(out);
}
template <class T>
std::optional<T> LoadVisualAsset(std::istream& in, std::string& error) {
    T value;
    std::string header;
    int version{};
    bool valid = false;
    if (in >> header >> version; version == 1) {
        if constexpr (std::is_same_v<T, Material2D>) {
            unsigned r, g, b, a;
            if (header == "PIPEFRAME_MATERIAL2D" &&
                (in >> std::quoted(value.texture.assetId) >> r >> g >> b >> a >> value.uvScale.x >> value.uvScale.y >>
                 value.smooth) &&
                r <= 255 && g <= 255 && b <= 255 && a <= 255) {
                value.tint = {std::uint8_t(r), std::uint8_t(g), std::uint8_t(b), std::uint8_t(a)};
                valid = true;
            }
        } else
            valid = header == "PIPEFRAME_TILESET2D" &&
                    bool(in >> std::quoted(value.material.assetId) >> value.tileWidth >> value.tileHeight >>
                         value.columns >> value.rows >> value.margin >> value.spacing);
    }
    if (!valid) {
        error = "Invalid visual asset format or version";
        return {};
    }
    in >> std::ws;
    if (!in.eof()) {
        error = "Unexpected trailing visual asset data";
        return {};
    }
    T checked;
    if (!T::Schema().Apply(checked, T::Schema().Serialize(value).properties, error)) return {};
    error.clear();
    return checked;
}
}  // namespace pipeframe
