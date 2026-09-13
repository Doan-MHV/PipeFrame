#include <PipeFrame/Backend/SFML/UI/NetworkView.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <unordered_map>
#include <SFML/Graphics/Text.hpp>

NetworkView::NetworkView(const UITheme &newTheme)
    : selectionColor(newTheme.textPrimary), theme(newTheme) {
    SetHitTestVisible(false);
    SetSize({240, 160});
}
NetworkView::NetworkView(const sf::Font &newFont, const UITheme &newTheme)
    : NetworkView(newTheme) {
    font = &newFont;
}

void NetworkView::SetGraph(std::vector<NetworkNode> newNodes, std::vector<NetworkEdge> newEdges) {
    std::unordered_map<std::uint64_t, std::size_t> ids;
    for (std::size_t i = 0; i < newNodes.size(); ++i) {
        const auto &node = newNodes[i];
        if (!std::isfinite(node.position.x) || !std::isfinite(node.position.y) ||
            node.position.x < 0 || node.position.x > 1 || node.position.y < 0 || node.position.y > 1 ||
            !ids.emplace(node.id, i).second) {
            throw std::invalid_argument("Network nodes require unique IDs and finite positions in [0,1]");
        }
    }
    for (const auto &edge : newEdges) {
        if (!ids.contains(edge.source) || !ids.contains(edge.target) || edge.source == edge.target) {
            throw std::invalid_argument("Network edges require distinct, existing endpoints");
        }
    }
    if (selected && !ids.contains(*selected)) selected.reset();
    nodes = std::move(newNodes);
    edges = std::move(newEdges);
    RebuildGeometry();
}

const std::vector<NetworkNode> &NetworkView::GetNodes() const { return nodes; }
const std::vector<NetworkEdge> &NetworkView::GetEdges() const { return edges; }
void NetworkView::SetNodeRadius(const float value) {
    if (!std::isfinite(value) || value <= 0) throw std::invalid_argument("Node radius must be finite and positive");
    radius = value;
    RebuildGeometry();
}
void NetworkView::SetSelectedNode(const std::optional<std::uint64_t> id) {
    if (id && std::none_of(nodes.begin(), nodes.end(), [&](const auto &node) { return node.id == *id; })) {
        throw std::invalid_argument("Selected network node must exist");
    }
    selected = id;
    RebuildGeometry();
}
std::optional<std::uint64_t> NetworkView::GetSelectedNode() const { return selected; }
std::optional<std::uint64_t> NetworkView::FindNodeAt(const sf::Vector2f point) const {
    for (const Widget *widget = this; widget; widget = widget->GetParent()) {
        if (!widget->IsVisible() || !widget->IsEnabled()) return std::nullopt;
    }
    if (visualRadius <= 0) return std::nullopt;
    for (std::size_t i = centers.size(); i > 0; --i) {
        const auto delta = point - centers[i - 1];
        if (delta.x * delta.x + delta.y * delta.y <= visualRadius * visualRadius) return nodes[i - 1].id;
    }
    return std::nullopt;
}
void NetworkView::OnGeometryChanged() { RebuildGeometry(); }
void NetworkView::OnOpacityChanged() { RebuildGeometry(); }
void NetworkView::OnRender(sf::RenderTarget &target) const {
    if (!vertices.empty()) target.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Triangles);
    if (!font)
        return;
    for (std::size_t index = 0; index < nodes.size() && index < centers.size(); ++index) {
        if (nodes[index].label.empty())
            continue;
        std::string caption = nodes[index].label;
        if (!nodes[index].value.empty())
            caption += "  " + nodes[index].value;
        sf::Text text(*font, caption, 10);
        sf::Color color = theme.textPrimary;
        color.a = static_cast<std::uint8_t>(color.a * GetEffectiveOpacity());
        text.setFillColor(color);
        const auto bounds = text.getLocalBounds();
        const bool rightAligned = nodes[index].position.x > 0.65f;
        text.setPosition({rightAligned ? centers[index].x - visualRadius - 7.0f - bounds.size.x
                                       : centers[index].x + visualRadius + 7.0f,
                          centers[index].y - bounds.size.y * 0.5f - bounds.position.y});
        target.draw(text);
    }
}

void NetworkView::RebuildGeometry() {
    vertices.clear(); centers.clear();
    const auto size = GetSize();
    visualRadius = std::max(0.0f, std::min(radius, std::min(size.x, size.y) * 0.5f - 3.0f));
    if (visualRadius <= 0) return;
    const float margin = visualRadius + 3.0f;
    const auto origin = GetScreenPosition() + sf::Vector2f{margin, margin};
    const auto extent = size - sf::Vector2f{margin * 2, margin * 2};
    std::unordered_map<std::uint64_t, std::size_t> ids;
    for (const auto &node : nodes) {
        ids.emplace(node.id, centers.size());
        centers.push_back(origin + sf::Vector2f{node.position.x * extent.x, node.position.y * extent.y});
    }
    const auto fade = [&](sf::Color color) {
        color.a = static_cast<std::uint8_t>(std::lround(color.a * GetEffectiveOpacity()));
        return color;
    };
    const auto triangle = [&](sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Color color) {
        vertices.emplace_back(a, color); vertices.emplace_back(b, color); vertices.emplace_back(c, color);
    };
    for (const auto &edge : edges) {
        const auto from = centers[ids.at(edge.source)], to = centers[ids.at(edge.target)];
        const auto delta = to - from;
        const float length = std::hypot(delta.x, delta.y);
        if (length <= 2 * (visualRadius + 3)) continue;
        const auto direction = delta / length;
        const sf::Vector2f normal{-direction.y, direction.x};
        const auto start = from + direction * (visualRadius + 3);
        const auto end = to - direction * (visualRadius + 3);
        const float arrow = edge.directed ? std::min(6.0f, (length - 2 * (visualRadius + 3)) * 0.5f) : 0;
        const auto base = end - direction * arrow;
        const auto color = fade(edge.color);
        const float halfWidth = std::clamp(edge.strength, 0.5f, 4.0f) * 0.75f;
        triangle(start + normal * halfWidth, start - normal * halfWidth,
                 base + normal * halfWidth, color);
        triangle(base + normal * halfWidth, start - normal * halfWidth,
                 base - normal * halfWidth, color);
        if (arrow > 0) triangle(end, base + normal * (arrow * 0.5f), base - normal * (arrow * 0.5f), color);
    }
    const auto disc = [&](sf::Vector2f center, float r, sf::Color color) {
        for (int i = 0; i < 24; ++i) {
            const float a = i * 2 * std::numbers::pi_v<float> / 24;
            const float b = (i + 1) * 2 * std::numbers::pi_v<float> / 24;
            triangle(center, center + sf::Vector2f{std::cos(a), std::sin(a)} * r,
                     center + sf::Vector2f{std::cos(b), std::sin(b)} * r, fade(color));
        }
    };
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (selected == nodes[i].id) disc(centers[i], visualRadius + 3, selectionColor);
        disc(centers[i], visualRadius, nodes[i].color);
    }
}
