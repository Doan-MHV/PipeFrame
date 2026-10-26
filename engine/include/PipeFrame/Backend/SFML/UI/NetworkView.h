#ifndef PIPEFRAME_UI_NETWORK_VIEW_H
#define PIPEFRAME_UI_NETWORK_VIEW_H

#include <PipeFrame/Backend/SFML/UI/UITheme.h>
#include <PipeFrame/Backend/SFML/UI/Widget.h>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct NetworkNode {
    std::uint64_t id;
    sf::Vector2f position;  // Normalized [0,1] coordinates; layout is application-owned.
    sf::Color color = UITheme::Dark().accent;
    std::string label;
    std::string value;
};

struct NetworkEdge {
    std::uint64_t source;
    std::uint64_t target;
    sf::Color color = UITheme::Dark().textSecondary;
    bool directed = true;
    float strength = 1.0f;
};

// Passive, domain-independent graph surface. Query a node for application-owned inspection.
class NetworkView final : public Widget {
public:
    explicit NetworkView(const UITheme& theme = UITheme::Dark());
    NetworkView(const sf::Font& font, const UITheme& theme = UITheme::Dark());
    // Invalid graphs throw without changing the display. Self edges are not supported.
    void SetGraph(std::vector<NetworkNode> nodes, std::vector<NetworkEdge> edges);
    const std::vector<NetworkNode>& GetNodes() const;
    const std::vector<NetworkEdge>& GetEdges() const;
    void SetNodeRadius(float radius);
    void SetSelectedNode(std::optional<std::uint64_t> id);
    std::optional<std::uint64_t> GetSelectedNode() const;
    std::optional<std::uint64_t> FindNodeAt(sf::Vector2f screenPoint) const;

protected:
    void OnRender(sf::RenderTarget& target) const override;
    void OnGeometryChanged() override;
    void OnOpacityChanged() override;

private:
    void RebuildGeometry();
    std::vector<NetworkNode> nodes;
    std::vector<NetworkEdge> edges;
    std::vector<sf::Vector2f> centers;
    std::vector<sf::Vertex> vertices;
    std::optional<std::uint64_t> selected;
    sf::Color selectionColor;
    float radius = 8.0f;
    float visualRadius = 0.0f;
    const sf::Font* font = nullptr;
    UITheme theme;
};

#endif
