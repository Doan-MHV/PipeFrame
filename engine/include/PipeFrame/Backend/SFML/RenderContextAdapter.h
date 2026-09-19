#pragma once

#include <PipeFrame/Render/RenderContext.h>

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

namespace pipeframe::backend::sfml {
// The native target must outlive the context and all copies of its surface.
RenderContext MakeRenderContext(sf::RenderWindow&);
RenderContext MakeRenderContext(sf::RenderTexture&);
sf::RenderTarget& GetTarget(const RenderContext&);
sf::RenderWindow& GetWindow(const RenderContext&);

inline sf::Vector2f ScreenToWorld(const RenderContext& context, sf::Vector2i p) {
    const auto v = context.MapPixelToWorld({p.x, p.y});
    return {v.x, v.y};
}

inline sf::Vector2i WorldToScreen(const RenderContext& context, sf::Vector2f p) {
    const auto v = context.MapWorldToPixel({p.x, p.y});
    return {v.x, v.y};
}

inline sf::IntRect ViewportBounds(const RenderContext& context) {
    const auto r = context.GetViewportRectangle();
    return {{r.position.x, r.position.y}, {r.size.x, r.size.y}};
}

}  // namespace pipeframe::backend::sfml
