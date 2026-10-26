#include "World/Rendering/AntRenderer.h"

#include <algorithm>
#include <utility>

namespace ant_simulation {

AntRenderer::AntRenderer(const AntConfiguration &newConfiguration) : configuration(newConfiguration) {}

bool AntRenderer::LoadAssets(const std::filesystem::path &assetRoot, std::string &errorMessage) {
    const std::filesystem::path textureRoot = assetRoot / "Textures";

    resources.Clear();
    circleTexture = resources.LoadTexture((textureRoot / "circle.png").string(), true, &errorMessage);
    bodyTexture = resources.LoadTexture((textureRoot / "ant_body_parts.png").string(), true, &errorMessage);
    fullAntTexture = resources.LoadTexture((textureRoot / "ant_full.png").string(), true, &errorMessage);
    legTexture = resources.LoadTexture((textureRoot / "ant_leg.png").string(), true, &errorMessage);
    if (resources.State(circleTexture) != pipeframe::ResourceState::Ready ||
        resources.State(bodyTexture) != pipeframe::ResourceState::Ready ||
        resources.State(fullAntTexture) != pipeframe::ResourceState::Ready ||
        resources.State(legTexture) != pipeframe::ResourceState::Ready) {
        assetsLoaded = false;
        return false;
    }

    assetsLoaded = true;
    errorMessage.clear();

    return true;
}

void AntRenderer::SetAutomaticMode(const bool enabled) { automaticMode = enabled; }

void AntRenderer::SetMode(const AntRenderingMode newMode) {
    mode = newMode;
    automaticMode = false;
}

void AntRenderer::UpdateGeometry(const std::span<const AntView> ants, const pipeframe::Rectanglef &viewport,
                                 const float zoom) {
    if (automaticMode) {
        mode = SelectMode(zoom);
    }

    statistics = {};
    statistics.mode = mode;
    statistics.candidateCount = ants.size();

    std::vector<const AntView *> visibleAnts;

    visibleAnts.reserve(std::min<std::size_t>(ants.size(), 65'536));

    const float margin = mode == AntRenderingMode::DetailedQuads ? 5.0f : 2.0f;

    for (const AntView &ant : ants) {
        if (ant.IsDead() || !IsVisible(ant.GetPosition(), viewport, margin)) {
            continue;
        }

        visibleAnts.push_back(&ant);
    }

    statistics.visibleCount = visibleAnts.size();

    pointVertices.clear();

    switch (mode) {
    case AntRenderingMode::DetailedQuads:
        geometry.ResizeDetailed(visibleAnts.size());

        for (std::size_t index = 0; index < visibleAnts.size(); ++index) {
            geometry.UpdateDetailed(*visibleAnts[index], index, configuration);
        }

        statistics.vertexCount =
            geometry.GetBodyVertices().size() + geometry.GetLegVertices().size() + geometry.GetFoodVertices().size();
        break;

    case AntRenderingMode::SimpleQuads:
        geometry.ResizeSimple(visibleAnts.size());

        for (std::size_t index = 0; index < visibleAnts.size(); ++index) {
            geometry.UpdateSimple(*visibleAnts[index], index, configuration);
        }

        statistics.vertexCount = geometry.GetBodyVertices().size() + geometry.GetFoodVertices().size();
        break;

    case AntRenderingMode::Points:
        pointVertices.resize(visibleAnts.size());

        for (std::size_t index = 0; index < visibleAnts.size(); ++index) {
            const AntView &ant = *visibleAnts[index];

            pointVertices[index].position = ant.GetPosition();

            pointVertices[index].color = AntGeometry::GetAntColor(ant, configuration);

            pointVertices[index].textureCoordinate = {};
        }

        statistics.vertexCount = pointVertices.size();
        break;
    }
}

void AntRenderer::Draw(pipeframe::Canvas target, pipeframe::RenderState states) const {
    switch (mode) {
    case AntRenderingMode::DetailedQuads:
        states.texture = assetsLoaded ? pipeframe::TextureBinding(resources, legTexture) : nullptr;

        DrawVertices(target, geometry.GetLegVertices(), pipeframe::PrimitiveTopology::Triangles, states);

        states.texture = assetsLoaded ? pipeframe::TextureBinding(resources, circleTexture) : nullptr;

        DrawVertices(target, geometry.GetFoodVertices(), pipeframe::PrimitiveTopology::Triangles, states);

        states.texture = assetsLoaded ? pipeframe::TextureBinding(resources, bodyTexture) : nullptr;

        DrawVertices(target, geometry.GetBodyVertices(), pipeframe::PrimitiveTopology::Triangles, states);
        break;

    case AntRenderingMode::SimpleQuads:
        states.texture = assetsLoaded ? pipeframe::TextureBinding(resources, circleTexture) : nullptr;

        DrawVertices(target, geometry.GetFoodVertices(), pipeframe::PrimitiveTopology::Triangles, states);

        states.texture = assetsLoaded ? pipeframe::TextureBinding(resources, fullAntTexture) : nullptr;

        DrawVertices(target, geometry.GetBodyVertices(), pipeframe::PrimitiveTopology::Triangles, states);
        break;

    case AntRenderingMode::Points:
        states.texture = nullptr;

        DrawVertices(target, pointVertices, pipeframe::PrimitiveTopology::Points, states);
        break;
    }
}

bool AntRenderer::AreAssetsLoaded() const { return assetsLoaded; }

AntRenderingMode AntRenderer::GetMode() const { return mode; }

const AntRendererStatistics &AntRenderer::GetStatistics() const { return statistics; }

const AntGeometry &AntRenderer::GetGeometry() const { return geometry; }

std::span<const pipeframe::Vertex2D> AntRenderer::GetPointVertices() const { return pointVertices; }

AntRenderingMode AntRenderer::SelectMode(const float zoom) const {
    if (zoom > configuration.detailZoomThreshold) {
        return AntRenderingMode::DetailedQuads;
    }

    if (zoom > PointZoomThreshold) {
        return AntRenderingMode::SimpleQuads;
    }

    return AntRenderingMode::Points;
}

bool AntRenderer::IsVisible(const pipeframe::Vector2f position, const pipeframe::Rectanglef &viewport,
                            const float margin) {
    const float minimumX = viewport.position.x - margin;

    const float minimumY = viewport.position.y - margin;

    const float maximumX = viewport.position.x + viewport.size.x + margin;

    const float maximumY = viewport.position.y + viewport.size.y + margin;

    return position.x >= minimumX && position.x <= maximumX && position.y >= minimumY && position.y <= maximumY;
}

void AntRenderer::DrawVertices(pipeframe::Canvas target, const std::span<const pipeframe::Vertex2D> vertices,
                               const pipeframe::PrimitiveTopology primitiveType, const pipeframe::RenderState &states) {
    if (vertices.empty()) {
        return;
    }

    target.Draw(vertices.data(), vertices.size(), primitiveType, states);
}

} // namespace ant_simulation
