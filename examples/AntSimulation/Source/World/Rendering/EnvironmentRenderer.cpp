#include "World/Rendering/EnvironmentRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace ant_simulation {

EnvironmentRenderer::EnvironmentRenderer(const AntConfiguration &newConfiguration)
    : configuration(newConfiguration), markerRenderer(newConfiguration) {}

bool EnvironmentRenderer::LoadAssets(const std::filesystem::path &assetRoot, std::string &errorMessage) {
    const std::filesystem::path circlePath = assetRoot / "Textures" / "circle.png";

    resources.Clear();circleTexture=resources.LoadTexture(circlePath.string(),true,&errorMessage);
    if(resources.State(circleTexture)!=pipeframe::ResourceState::Ready){assetsLoaded=false;return false;}

    if (!markerRenderer.LoadAssets(assetRoot, errorMessage)) {
        assetsLoaded = false;
        return false;
    }

    assetsLoaded = true;
    errorMessage.clear();

    return true;
}

void EnvironmentRenderer::SetGridEnabled(const bool enabled) {
    gridEnabled = enabled;

    if (!gridEnabled) {
        gridVertices.clear();
    }
}

void EnvironmentRenderer::SetMarkersEnabled(const bool enabled) { markerRenderer.SetEnabled(enabled); }

void EnvironmentRenderer::SetMarkerColorPower(const float colorPower) { markerRenderer.SetColorPower(colorPower); }

void EnvironmentRenderer::SetWallShadowEnabled(const bool enabled) { wallRenderer.SetShadowEnabled(enabled); }

void EnvironmentRenderer::Update(const AntEnvironment &environment, const std::span<const ColonyView> colonies,
                                 const pipeframe::Rectanglef &viewport) {
    BuildBackground(environment);
    BuildGrid(environment);

    BuildColonies(colonies, viewport);

    markerRenderer.Update(environment, viewport);

    BuildFood(environment, viewport);

    wallRenderer.Rebuild(environment, viewport);
}

void EnvironmentRenderer::Draw(pipeframe::Canvas target, pipeframe::RenderState states) const {
    states.texture = nullptr;
    states.blendMode = pipeframe::BlendMode::Alpha;

    if(authoredGround){auto surface=*authoredGround;surface.showGrid=false;pipeframe::DrawPlayground(target,{},surface,groundState,groundUv);}
    else if (!backgroundVertices.empty()) {
        target.Draw(backgroundVertices.data(), backgroundVertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
    }

    if (gridEnabled && (!authoredGround||authoredGround->showGrid) && !gridVertices.empty()) {
        target.Draw(gridVertices.data(), gridVertices.size(), pipeframe::PrimitiveTopology::Lines, states);
    }

    if (!colonyShadowVertices.empty()) {
        target.Draw(colonyShadowVertices.data(), colonyShadowVertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
    }

    if (!colonyFillVertices.empty()) {
        target.Draw(colonyFillVertices.data(), colonyFillVertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
    }

    if (!colonyOutlineVertices.empty()) {
        target.Draw(colonyOutlineVertices.data(), colonyOutlineVertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
    }

    markerRenderer.Draw(target, states);

    if (terrainVisible && !foodVertices.empty()) {
        pipeframe::RenderState foodStates = states;

        foodStates.texture=assetsLoaded?pipeframe::TextureBinding(resources,circleTexture):nullptr;

        target.Draw(foodVertices.data(), foodVertices.size(), pipeframe::PrimitiveTopology::Triangles, foodStates);
    }

    if(terrainVisible)wallRenderer.Draw(target, states);
}

bool EnvironmentRenderer::AreAssetsLoaded() const { return assetsLoaded; }

bool EnvironmentRenderer::IsGridEnabled() const { return gridEnabled; }

std::size_t EnvironmentRenderer::GetVisibleFoodCount() const { return visibleFoodCount; }

std::size_t EnvironmentRenderer::GetVisibleColonyCount() const { return visibleColonyCount; }

std::span<const pipeframe::Vertex2D> EnvironmentRenderer::GetBackgroundVertices() const { return backgroundVertices; }

std::span<const pipeframe::Vertex2D> EnvironmentRenderer::GetGridVertices() const { return gridVertices; }

std::span<const pipeframe::Vertex2D> EnvironmentRenderer::GetFoodVertices() const { return foodVertices; }

std::span<const pipeframe::Vertex2D> EnvironmentRenderer::GetColonyFillVertices() const { return colonyFillVertices; }

std::span<const pipeframe::Vertex2D> EnvironmentRenderer::GetColonyOutlineVertices() const { return colonyOutlineVertices; }

std::span<const pipeframe::Vertex2D> EnvironmentRenderer::GetColonyShadowVertices() const { return colonyShadowVertices; }

const MarkerRenderer &EnvironmentRenderer::GetMarkerRenderer() const { return markerRenderer; }

const WallRenderer &EnvironmentRenderer::GetWallRenderer() const { return wallRenderer; }

void EnvironmentRenderer::BuildBackground(const AntEnvironment &environment) {
    backgroundVertices.clear();
    backgroundVertices.reserve(6);

    const pipeframe::Vector2f worldSize{
        static_cast<float>(environment.GetWidth()),
        static_cast<float>(environment.GetHeight()),
    };

    AddQuad(backgroundVertices, worldSize * 0.5f, worldSize * 0.5f, BackgroundColor, false);
}

void EnvironmentRenderer::BuildGrid(const AntEnvironment &environment) {
    gridVertices.clear();

    if (!gridEnabled) {
        return;
    }

    const int width = environment.GetWidth();

    const int height = environment.GetHeight();

    gridVertices.reserve(static_cast<std::size_t>(width + height + 2) * 2);

    for (int x = 0; x <= width; ++x) {
        const pipeframe::Color color = x % 8 == 0 ? MajorGridColor : GridColor;

        pipeframe::Vertex2D top;
        top.position = {
            static_cast<float>(x),
            0.0f,
        };
        top.color = color;

        pipeframe::Vertex2D bottom;
        bottom.position = {
            static_cast<float>(x),
            static_cast<float>(height),
        };
        bottom.color = color;

        gridVertices.push_back(top);
        gridVertices.push_back(bottom);
    }

    for (int y = 0; y <= height; ++y) {
        const pipeframe::Color color = y % 8 == 0 ? MajorGridColor : GridColor;

        pipeframe::Vertex2D left;
        left.position = {
            0.0f,
            static_cast<float>(y),
        };
        left.color = color;

        pipeframe::Vertex2D right;
        right.position = {
            static_cast<float>(width),
            static_cast<float>(y),
        };
        right.color = color;

        gridVertices.push_back(left);
        gridVertices.push_back(right);
    }
}

void EnvironmentRenderer::BuildFood(const AntEnvironment &environment, const pipeframe::Rectanglef &viewport) {
    foodVertices.clear();
    visibleFoodCount = 0;

    const std::span<const Food> foodEntities = environment.GetFoodEntities();

    foodVertices.reserve(foodEntities.size() * 6);

    for (const Food &food : foodEntities) {
        const pipeframe::Vector2f position = food.position;
        if (!IsVisible(position, FoodHalfSize, viewport)) {
            continue;
        }

        AddQuad(foodVertices, position,
                {
                    FoodHalfSize,
                    FoodHalfSize,
                },
                configuration.foodColor, true);

        ++visibleFoodCount;
    }
}

void EnvironmentRenderer::BuildColonies(const std::span<const ColonyView> colonies, const pipeframe::Rectanglef &viewport) {
    colonyFillVertices.clear();
    colonyOutlineVertices.clear();
    colonyShadowVertices.clear();

    visibleColonyCount = 0;

    colonyFillVertices.reserve(colonies.size() * ColonySegmentCount * 3);

    colonyOutlineVertices.reserve(colonies.size() * ColonySegmentCount * 6);

    colonyShadowVertices.reserve(colonies.size() * ColonySegmentCount * 3);

    for (const ColonyView &colony : colonies) {
        const float radius = colony.GetRadius();

        const pipeframe::Vector2f position = colony.GetPosition();
        if (radius <= 0.0f || !IsVisible(position, radius + ColonyShadowOffset.y, viewport)) {
            continue;
        }

        const pipeframe::Color color = colony.GetColor();

        const float outlineThickness = radius * 0.2f;

        const float innerRadius = std::max(0.0f, radius - outlineThickness);

        AddCircle(colonyShadowVertices, position + ColonyShadowOffset, radius, ColonyShadowColor);

        AddCircle(colonyFillVertices, position, radius, color);

        AddRing(colonyOutlineVertices, position, radius, innerRadius, color);

        ++visibleColonyCount;
    }
}

void EnvironmentRenderer::AddQuad(std::vector<pipeframe::Vertex2D> &vertices, const pipeframe::Vector2f center,
                                  const pipeframe::Vector2f halfSize, const pipeframe::Color color, const bool textured) {
    const pipeframe::Vector2f northWest{
        center.x - halfSize.x,
        center.y - halfSize.y,
    };

    const pipeframe::Vector2f northEast{
        center.x + halfSize.x,
        center.y - halfSize.y,
    };

    const pipeframe::Vector2f southWest{
        center.x - halfSize.x,
        center.y + halfSize.y,
    };

    const pipeframe::Vector2f southEast{
        center.x + halfSize.x,
        center.y + halfSize.y,
    };

    const float textureMaximum = textured ? 1024.0f : 0.0f;

    const std::array<pipeframe::Vector2f, 6> positions{
        northWest, northEast, southWest, southWest, southEast, northEast,
    };

    const std::array<pipeframe::Vector2f, 6> textureCoordinates{
        pipeframe::Vector2f{0.0f, 0.0f},
        pipeframe::Vector2f{textureMaximum, 0.0f},
        pipeframe::Vector2f{0.0f, textureMaximum},
        pipeframe::Vector2f{0.0f, textureMaximum},
        pipeframe::Vector2f{
            textureMaximum,
            textureMaximum,
        },
        pipeframe::Vector2f{
            textureMaximum,
            0.0f,
        },
    };

    for (std::size_t index = 0; index < positions.size(); ++index) {
        pipeframe::Vertex2D vertex;

        vertex.position = positions[index];

        vertex.color = color;

        vertex.textureCoordinate = textureCoordinates[index];

        vertices.push_back(vertex);
    }
}

void EnvironmentRenderer::AddCircle(std::vector<pipeframe::Vertex2D> &vertices, const pipeframe::Vector2f center, const float radius,
                                    const pipeframe::Color color) {
    if (radius <= 0.0f) {
        return;
    }

    constexpr float fullRotation = 2.0f * std::numbers::pi_v<float>;

    for (std::size_t segment = 0; segment < ColonySegmentCount; ++segment) {
        const float firstAngle = fullRotation * static_cast<float>(segment) / static_cast<float>(ColonySegmentCount);

        const float secondAngle =
            fullRotation * static_cast<float>(segment + 1) / static_cast<float>(ColonySegmentCount);

        const pipeframe::Vector2f first{
            center.x + std::cos(firstAngle) * radius,
            center.y + std::sin(firstAngle) * radius,
        };

        const pipeframe::Vector2f second{
            center.x + std::cos(secondAngle) * radius,
            center.y + std::sin(secondAngle) * radius,
        };

        pipeframe::Vertex2D centerVertex;
        centerVertex.position = center;
        centerVertex.color = color;

        pipeframe::Vertex2D firstVertex;
        firstVertex.position = first;
        firstVertex.color = color;

        pipeframe::Vertex2D secondVertex;
        secondVertex.position = second;
        secondVertex.color = color;

        vertices.push_back(centerVertex);
        vertices.push_back(firstVertex);
        vertices.push_back(secondVertex);
    }
}

void EnvironmentRenderer::AddRing(std::vector<pipeframe::Vertex2D> &vertices, const pipeframe::Vector2f center, const float outerRadius,
                                  const float innerRadius, const pipeframe::Color color) {
    if (outerRadius <= 0.0f || innerRadius < 0.0f || innerRadius >= outerRadius) {
        return;
    }

    constexpr float fullRotation = 2.0f * std::numbers::pi_v<float>;

    for (std::size_t segment = 0; segment < ColonySegmentCount; ++segment) {
        const float firstAngle = fullRotation * static_cast<float>(segment) / static_cast<float>(ColonySegmentCount);

        const float secondAngle =
            fullRotation * static_cast<float>(segment + 1) / static_cast<float>(ColonySegmentCount);

        const pipeframe::Vector2f outerFirst{
            center.x + std::cos(firstAngle) * outerRadius,
            center.y + std::sin(firstAngle) * outerRadius,
        };

        const pipeframe::Vector2f outerSecond{
            center.x + std::cos(secondAngle) * outerRadius,
            center.y + std::sin(secondAngle) * outerRadius,
        };

        const pipeframe::Vector2f innerFirst{
            center.x + std::cos(firstAngle) * innerRadius,
            center.y + std::sin(firstAngle) * innerRadius,
        };

        const pipeframe::Vector2f innerSecond{
            center.x + std::cos(secondAngle) * innerRadius,
            center.y + std::sin(secondAngle) * innerRadius,
        };

        const std::array<pipeframe::Vector2f, 6> positions{
            outerFirst, outerSecond, innerFirst, innerFirst, outerSecond, innerSecond,
        };

        for (const pipeframe::Vector2f position : positions) {
            pipeframe::Vertex2D vertex;
            vertex.position = position;
            vertex.color = color;

            vertices.push_back(vertex);
        }
    }
}

bool EnvironmentRenderer::IsVisible(const pipeframe::Vector2f center, const float radius, const pipeframe::Rectanglef &viewport) {
    const float viewportLeft = viewport.position.x;

    const float viewportTop = viewport.position.y;

    const float viewportRight = viewport.position.x + viewport.size.x;

    const float viewportBottom = viewport.position.y + viewport.size.y;

    return center.x + radius >= viewportLeft && center.x - radius <= viewportRight &&
           center.y + radius >= viewportTop && center.y - radius <= viewportBottom;
}

} // namespace ant_simulation
