#include "World/Rendering/WallRenderer.h"

#include <array>

#include "World/Runtime/Environment/AntWorldCell.h"

namespace ant_simulation {

void WallRenderer::SetShadowEnabled(const bool enabled) {
    shadowEnabled = enabled;

    if (!shadowEnabled) {
        shadowVertices.clear();
    }
}

void WallRenderer::Rebuild(const AntEnvironment &environment, const pipeframe::Rectanglef &viewport) {
    wallVertices.clear();
    shadowVertices.clear();

    candidateCount = environment.GetWallCount();

    visibleWallCount = 0;

    wallVertices.reserve(candidateCount * 6);

    if (shadowEnabled) {
        shadowVertices.reserve(candidateCount * 6);
    }

    for (int y = 0; y < environment.GetHeight(); ++y) {
        for (int x = 0; x < environment.GetWidth(); ++x) {
            const AntWorldCell *cell = environment.TryGetCell(x, y);

            if (cell == nullptr || !cell->wall) {
                continue;
            }

            const pipeframe::Vector2f position{
                static_cast<float>(x),
                static_cast<float>(y),
            };

            if (!IsVisible(position, viewport)) {
                continue;
            }

            const WallType type = WallBuilder::GetWallType(environment, {x, y});

            AddWall(position, type);

            ++visibleWallCount;
        }
    }
}

void WallRenderer::Draw(pipeframe::Canvas target, pipeframe::RenderState states) const {
    states.texture = nullptr;

    if (shadowEnabled && !shadowVertices.empty()) {
        pipeframe::RenderState shadowStates = states;

        shadowStates.blendMode = pipeframe::BlendMode::Alpha;

        target.Draw(shadowVertices.data(), shadowVertices.size(), pipeframe::PrimitiveTopology::Triangles,
                    shadowStates);
    }

    if (wallVertices.empty()) {
        return;
    }

    target.Draw(wallVertices.data(), wallVertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
}

bool WallRenderer::IsShadowEnabled() const { return shadowEnabled; }

std::size_t WallRenderer::GetCandidateCount() const { return candidateCount; }

std::size_t WallRenderer::GetVisibleWallCount() const { return visibleWallCount; }

std::span<const pipeframe::Vertex2D> WallRenderer::GetWallVertices() const { return wallVertices; }

std::span<const pipeframe::Vertex2D> WallRenderer::GetShadowVertices() const { return shadowVertices; }

void WallRenderer::AddWall(const pipeframe::Vector2f position, const WallType type) {
    AddShape(wallVertices, position, type, WallColor, {});

    if (shadowEnabled) {
        AddShape(shadowVertices, position, type, ShadowColor, ShadowOffset);
    }
}

void WallRenderer::AddShape(std::vector<pipeframe::Vertex2D> &vertices, const pipeframe::Vector2f position,
                            const WallType type, const pipeframe::Color color, const pipeframe::Vector2f offset) {
    const pipeframe::Vector2f p = position + offset;

    const pipeframe::Vector2f northWest = p;

    const pipeframe::Vector2f northEast = p + pipeframe::Vector2f{
                                                  1.0f,
                                                  0.0f,
                                              };

    const pipeframe::Vector2f southWest = p + pipeframe::Vector2f{
                                                  0.0f,
                                                  1.0f,
                                              };

    const pipeframe::Vector2f southEast = p + pipeframe::Vector2f{
                                                  1.0f,
                                                  1.0f,
                                              };

    std::array<pipeframe::Vector2f, 6> positions{};

    switch (type) {
    case WallType::NorthWest:
        positions = {
            p, p, p, southWest, southEast, northEast,
        };
        break;

    case WallType::NorthEast:
        positions = {
            p, p, p, northWest, southWest, southEast,
        };
        break;

    case WallType::SouthWest:
        positions = {
            p, p, p, northWest, northEast, southEast,
        };
        break;

    case WallType::SouthEast:
        positions = {
            p, p, p, northWest, northEast, southWest,
        };
        break;

    case WallType::Full:
        positions = {
            northWest, northEast, southWest, southWest, southEast, northEast,
        };
        break;
    }

    for (const pipeframe::Vector2f vertexPosition : positions) {
        pipeframe::Vertex2D vertex;

        vertex.position = vertexPosition;

        vertex.color = color;
        vertex.textureCoordinate = {};

        vertices.push_back(vertex);
    }
}

bool WallRenderer::IsVisible(const pipeframe::Vector2f position, const pipeframe::Rectanglef &viewport) {
    const float cellMaximumX = position.x + 1.0f;

    const float cellMaximumY = position.y + 1.0f;

    const float viewportMaximumX = viewport.position.x + viewport.size.x;

    const float viewportMaximumY = viewport.position.y + viewport.size.y;

    return cellMaximumX >= viewport.position.x && cellMaximumY >= viewport.position.y &&
           position.x <= viewportMaximumX && position.y <= viewportMaximumY;
}

} // namespace ant_simulation