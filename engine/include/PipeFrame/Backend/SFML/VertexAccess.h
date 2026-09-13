#pragma once
#include <PipeFrame/Render/RenderTypes.h>
#include <PipeFrame/Backend/SFML/Conversions.h>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <span>
#include <vector>
namespace pipeframe::backend::sfml {
// Backend adapter for neutral geometry; the solver/geometry producer never
// constructs backend vertices. Legacy draw hosts use this until host migration.
inline void ConvertVertices(std::span<const Vertex2D> source, std::vector<sf::Vertex> &result) {
    result.resize(source.size());
    for (std::size_t i = 0; i < source.size(); ++i)
        result[i] = {ToBackend(source[i].position), ToBackend(source[i].color),
                     ToBackend(source[i].textureCoordinate)};
}
inline void DrawVertices(sf::RenderTarget &target, std::span<const Vertex2D> source,
                         sf::PrimitiveType primitive, const sf::RenderStates &states) {
    thread_local std::vector<sf::Vertex> vertices;
    ConvertVertices(source, vertices);
    if (!vertices.empty()) target.draw(vertices.data(), vertices.size(), primitive, states);
}
}
