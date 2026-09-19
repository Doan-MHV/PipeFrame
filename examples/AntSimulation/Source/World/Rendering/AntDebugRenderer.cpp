#include "World/Rendering/AntDebugRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace ant_simulation {

void AntDebugRenderer::SetSelectedAnt(const std::optional<AntId> antId) {
    selectedAntId = antId;

    if (!selectedAntId.has_value()) {
        selectionVertices.clear();
        targetVertices.clear();
    }
}

void AntDebugRenderer::SetTargetVisible(const bool visible) {
    targetVisible = visible;

    if (!targetVisible) {
        targetVertices.clear();
    }
}

void AntDebugRenderer::SetPhysicsDebugVisible(const bool visible) {
    physicsDebugVisible = visible;

    if (!physicsDebugVisible) {
        physicsVertices.clear();
        visiblePhysicsBodyCount = 0;
    }
}

void AntDebugRenderer::Update(const std::span<const AntView> ants, const std::span<const AntPhysicsBody> bodies,
                              const pipeframe::Rectanglef &viewport) {
    selectionVertices.clear();
    targetVertices.clear();
    physicsVertices.clear();

    physicsCandidateCount = physicsDebugVisible ? bodies.size() : 0;

    visiblePhysicsBodyCount = 0;

    if (selectedAntId.has_value()) {
        const auto iterator = std::find_if(ants.begin(), ants.end(),
                                           [this](const AntView &ant) { return ant.GetId() == *selectedAntId; });

        if (iterator != ants.end() && !iterator->IsDead()) {
            const AntView &selectedAnt = *iterator;

            if (IsVisible(selectedAnt.GetPosition(), SelectionRadius, viewport)) {
                AddRing(selectionVertices, selectedAnt.GetPosition(), SelectionRadius,
                        SelectionRadius - SelectionThickness, SelectionColor, CircleSegmentCount);
            }

            if (targetVisible && IsVisible(selectedAnt.GetTarget(), TargetRadius, viewport)) {
                AddCircle(targetVertices, selectedAnt.GetTarget(), TargetRadius, TargetColor, CircleSegmentCount);
            }
        }
    }

    if (!physicsDebugVisible) {
        return;
    }

    physicsVertices.reserve(bodies.size() * PhysicsCircleSegmentCount * 3);

    pipeframe::VisitBodyDebugGeometry<AntPhysicsBody>(
        bodies, [](const AntPhysicsBody &body) { return body.moving ? MovingPhysicsColor : StaticPhysicsColor; },
        [this, &viewport](const pipeframe::DebugCircle2D &debugCircle) {
            const pipeframe::Vector2f position = debugCircle.circle.center;
            if (!IsVisible(position, PhysicsBodyRadius, viewport)) {
                return;
            }

            AddCircle(physicsVertices, position, PhysicsBodyRadius, debugCircle.color, PhysicsCircleSegmentCount);

            ++visiblePhysicsBodyCount;
        });
}

void AntDebugRenderer::Draw(pipeframe::Canvas target, pipeframe::RenderState states) const {
    states.texture = nullptr;
    states.blendMode = pipeframe::BlendMode::Alpha;

    if (!physicsVertices.empty()) {
        target.Draw(physicsVertices.data(), physicsVertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
    }

    if (!targetVertices.empty()) {
        target.Draw(targetVertices.data(), targetVertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
    }

    if (!selectionVertices.empty()) {
        target.Draw(selectionVertices.data(), selectionVertices.size(), pipeframe::PrimitiveTopology::Triangles,
                    states);
    }
}

std::optional<AntId> AntDebugRenderer::GetSelectedAnt() const { return selectedAntId; }

bool AntDebugRenderer::IsTargetVisible() const { return targetVisible; }

bool AntDebugRenderer::IsPhysicsDebugVisible() const { return physicsDebugVisible; }

std::size_t AntDebugRenderer::GetPhysicsCandidateCount() const { return physicsCandidateCount; }

std::size_t AntDebugRenderer::GetVisiblePhysicsBodyCount() const { return visiblePhysicsBodyCount; }

std::span<const pipeframe::Vertex2D> AntDebugRenderer::GetSelectionVertices() const { return selectionVertices; }

std::span<const pipeframe::Vertex2D> AntDebugRenderer::GetTargetVertices() const { return targetVertices; }

std::span<const pipeframe::Vertex2D> AntDebugRenderer::GetPhysicsVertices() const { return physicsVertices; }

void AntDebugRenderer::AddCircle(std::vector<pipeframe::Vertex2D> &vertices, const pipeframe::Vector2f center,
                                 const float radius, const pipeframe::Color color, const std::size_t segmentCount) {
    if (radius <= 0.0f || segmentCount < 3) {
        return;
    }

    constexpr float fullRotation = 2.0f * std::numbers::pi_v<float>;

    for (std::size_t segment = 0; segment < segmentCount; ++segment) {
        const float firstAngle = fullRotation * static_cast<float>(segment) / static_cast<float>(segmentCount);

        const float secondAngle = fullRotation * static_cast<float>(segment + 1) / static_cast<float>(segmentCount);

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

void AntDebugRenderer::AddRing(std::vector<pipeframe::Vertex2D> &vertices, const pipeframe::Vector2f center,
                               const float outerRadius, const float innerRadius, const pipeframe::Color color,
                               const std::size_t segmentCount) {
    if (outerRadius <= 0.0f || innerRadius < 0.0f || innerRadius >= outerRadius || segmentCount < 3) {
        return;
    }

    constexpr float fullRotation = 2.0f * std::numbers::pi_v<float>;

    for (std::size_t segment = 0; segment < segmentCount; ++segment) {
        const float firstAngle = fullRotation * static_cast<float>(segment) / static_cast<float>(segmentCount);

        const float secondAngle = fullRotation * static_cast<float>(segment + 1) / static_cast<float>(segmentCount);

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

bool AntDebugRenderer::IsVisible(const pipeframe::Vector2f center, const float radius,
                                 const pipeframe::Rectanglef &viewport) {
    const float left = viewport.position.x;

    const float top = viewport.position.y;

    const float right = viewport.position.x + viewport.size.x;

    const float bottom = viewport.position.y + viewport.size.y;

    return center.x + radius >= left && center.x - radius <= right && center.y + radius >= top &&
           center.y - radius <= bottom;
}

} // namespace ant_simulation
