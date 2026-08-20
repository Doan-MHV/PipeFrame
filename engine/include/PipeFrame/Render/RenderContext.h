#ifndef PIPEFRAME_RENDER_CONTEXT_H
#define PIPEFRAME_RENDER_CONTEXT_H

#include "PipeFrame/Render/Camera2D.h"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>

class RenderContext {
  public:
    explicit RenderContext(sf::RenderWindow &window) : window(window) { SetScreenSize(window.getSize()); }

    sf::RenderWindow &GetWindow() { return window; }

    const sf::RenderWindow &GetWindow() const { return window; }

    Camera2D &GetCamera() { return camera; }

    const Camera2D &GetCamera() const { return camera; }

    sf::Vector2f ScreenToWorld(sf::Vector2i pixelPosition) const {
        return window.mapPixelToCoords(pixelPosition, camera.GetView());
    }

    sf::Vector2i WorldToScreen(sf::Vector2f worldPosition) const {
        return window.mapCoordsToPixel(worldPosition, camera.GetView());
    }

    void BeginWorld() { window.setView(camera.GetView()); }

    void BeginScreen() { window.setView(screenView); }

    void SetScreenSize(sf::Vector2u newSize) {
        const sf::Vector2f screenSize{static_cast<float>(newSize.x), static_cast<float>(newSize.y)};

        screenView.setSize(screenSize);
        screenView.setCenter(screenSize * 0.5f);
    }

    sf::IntRect GetWorldViewportBounds() const { return window.getViewport(camera.GetView()); }

    bool IsInsideWorldViewport(sf::Vector2i pixelPosition) const {
        return GetWorldViewportBounds().contains(pixelPosition);
    }

  private:
    sf::RenderWindow &window;
    Camera2D camera;
    sf::View screenView;
};

#endif
