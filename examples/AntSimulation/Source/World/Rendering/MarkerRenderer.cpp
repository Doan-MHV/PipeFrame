#include "World/Rendering/MarkerRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "World/Runtime/Environment/Marker.h"

namespace ant_simulation {

MarkerRenderer::MarkerRenderer(const AntConfiguration &newConfiguration) : configuration(newConfiguration) {}

bool MarkerRenderer::LoadAssets(const std::filesystem::path &assetRoot, std::string &errorMessage) {
    const std::filesystem::path path = assetRoot / "Textures" / "marker.png";

    resources.Clear();markerTexture=resources.LoadTexture(path.string(),true,&errorMessage);
    assetsLoaded=resources.State(markerTexture)==pipeframe::ResourceState::Ready;
    if(!assetsLoaded)return false;
    errorMessage.clear();

    return true;
}

void MarkerRenderer::SetEnabled(const bool newEnabled) {
    enabled = newEnabled;

    if (!enabled) {
        vertices.clear();
        visibleMarkerCount = 0;
    }
}

void MarkerRenderer::SetColorPower(const float colorPower) {
    markerColorPower = std::clamp(colorPower, 0.025f, 0.5f);

    /*
     * Force the next Update() call to rebuild the marker
     * geometry using the new intensity.
     */
    frameCount = 0;
}

void MarkerRenderer::Update(const AntEnvironment &environment, const pipeframe::Rectanglef &viewport) {
    if (!enabled) {
        return;
    }

    if (frameCount % UpdateDecimation == 0) {
        ForceUpdate(environment, viewport);
    }

    ++frameCount;
}

void MarkerRenderer::ForceUpdate(const AntEnvironment &environment, const pipeframe::Rectanglef &viewport) {
    vertices.clear();

    candidateCount = environment.GetCellCount();

    visibleMarkerCount = 0;

    const int width = environment.GetWidth();

    const std::span<const AntWorldCell> cells = environment.GetCells();

    vertices.reserve(std::min<std::size_t>(cells.size() * 6, 1'000'000));

    for (std::size_t index = 0; index < cells.size(); ++index) {
        const AntWorldCell &cell = cells[index];

        // This matches AntPezza: markers underneath food are
        // hidden so the food remains readable.
        if (cell.foodQuantity > 0) {
            continue;
        }

        const pipeframe::Color color = GetCellColor(cell);

        if (color.a == 0) {
            continue;
        }

        const int x = static_cast<int>(index % static_cast<std::size_t>(width));

        const int y = static_cast<int>(index / static_cast<std::size_t>(width));

        const pipeframe::Vector2f center = AntEnvironment::GetCellCenter({
            x,
            y,
        });

        if (!IsVisible(center, viewport)) {
            continue;
        }

        AddMarkerQuad(center, color);

        ++visibleMarkerCount;
    }
}

void MarkerRenderer::Draw(pipeframe::Canvas target, pipeframe::RenderState states) const {
    if (!enabled || vertices.empty()) {
        return;
    }

    states.texture=assetsLoaded?pipeframe::TextureBinding(resources,markerTexture):nullptr;

    states.blendMode = pipeframe::BlendMode::Add;

    target.Draw(vertices.data(), vertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
}

bool MarkerRenderer::IsEnabled() const { return enabled; }

bool MarkerRenderer::AreAssetsLoaded() const { return assetsLoaded; }

std::size_t MarkerRenderer::GetCandidateCount() const { return candidateCount; }

std::size_t MarkerRenderer::GetVisibleMarkerCount() const { return visibleMarkerCount; }

std::span<const pipeframe::Vertex2D> MarkerRenderer::GetVertices() const { return vertices; }

pipeframe::Color MarkerRenderer::GetCellColor(const AntWorldCell &cell) const {
    const Marker &homeMarker = cell.GetMarker(MarkerKind::ToHome);

    const Marker &foodMarker = cell.GetMarker(MarkerKind::ToFood);

    if (!homeMarker.HasOwner() && !foodMarker.HasOwner()) {
        return pipeframe::Color::Transparent;
    }

    const auto markerAlpha = [this](const Marker &marker) {
        if (!marker.HasOwner() || marker.intensity <= 0.0f) {
            return 0.0f;
        }

        const float normalized = marker.intensity * configuration.GetMarkerMaximumIntensityInverse();

        return std::pow(std::max(0.0f, normalized), markerColorPower);
    };

    const float homeAlpha = 0.95f * markerAlpha(homeMarker);

    const float foodAlpha = 0.95f * markerAlpha(foodMarker);

    const float homeRed = static_cast<float>(configuration.toHomeMarkerColor.r) * 0.5f;

    const float homeGreen = static_cast<float>(configuration.toHomeMarkerColor.g) * 0.5f;

    const float homeBlue = static_cast<float>(configuration.toHomeMarkerColor.b) * 0.5f;

    const float foodRed = static_cast<float>(configuration.toFoodMarkerColor.r) * 0.35f;

    const float foodGreen = static_cast<float>(configuration.toFoodMarkerColor.g) * 0.35f;

    const float foodBlue = static_cast<float>(configuration.toFoodMarkerColor.b) * 0.35f;

    return {
        ToColorChannel(foodRed * foodAlpha + homeRed * homeAlpha),

        ToColorChannel(foodGreen * foodAlpha + homeGreen * homeAlpha),

        ToColorChannel(foodBlue * foodAlpha + homeBlue * homeAlpha),

        ToColorChannel(255.0f * std::min(foodAlpha + homeAlpha, 1.0f)),
    };
}

void MarkerRenderer::AddMarkerQuad(const pipeframe::Vector2f center, const pipeframe::Color color) {
    const pipeframe::Vector2f minimum = center - pipeframe::Vector2f{
                                              MarkerHalfSize,
                                              MarkerHalfSize,
                                          };

    const pipeframe::Vector2f maximum = center + pipeframe::Vector2f{
                                              MarkerHalfSize,
                                              MarkerHalfSize,
                                          };

    const std::array<pipeframe::Vector2f, 4> positions{
        pipeframe::Vector2f{minimum.x, minimum.y},
        pipeframe::Vector2f{maximum.x, minimum.y},
        pipeframe::Vector2f{maximum.x, maximum.y},
        pipeframe::Vector2f{minimum.x, maximum.y},
    };

    constexpr std::array<pipeframe::Vector2f, 4> textureCoordinates{
        pipeframe::Vector2f{0.0f, 0.0f},
        pipeframe::Vector2f{1024.0f, 0.0f},
        pipeframe::Vector2f{1024.0f, 1024.0f},
        pipeframe::Vector2f{0.0f, 1024.0f},
    };

    constexpr std::array<std::size_t, 6> indices{
        0, 1, 2, 0, 2, 3,
    };

    for (const std::size_t corner : indices) {
        pipeframe::Vertex2D vertex;

        vertex.position = positions[corner];

        vertex.textureCoordinate = textureCoordinates[corner];

        vertex.color = color;

        vertices.push_back(vertex);
    }
}

bool MarkerRenderer::IsVisible(const pipeframe::Vector2f position, const pipeframe::Rectanglef &viewport) {
    return position.x >= viewport.position.x - MarkerHalfSize && position.y >= viewport.position.y - MarkerHalfSize &&
           position.x <= viewport.position.x + viewport.size.x + MarkerHalfSize &&
           position.y <= viewport.position.y + viewport.size.y + MarkerHalfSize;
}

std::uint8_t MarkerRenderer::ToColorChannel(const float value) {
    return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 255.0f));
}

} // namespace ant_simulation
