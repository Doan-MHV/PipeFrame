#ifndef PIPEFRAME_RENDER_CONTEXT_H
#define PIPEFRAME_RENDER_CONTEXT_H

#include <SFML/Graphics/RenderWindow.hpp>

#include "PipeFrame/Render/Camera2D.h"

class RenderContext
{
public:
    explicit RenderContext(sf::RenderWindow& window)
        : window(window)
    {
    }

    sf::RenderWindow& GetWindow()
    {
        return window;
    }

    const sf::RenderWindow& GetWindow() const
    {
        return window;
    }

    Camera2D& GetCamera()
    {
        return camera;
    }

    const Camera2D& GetCamera() const
    {
        return camera;
    }

    sf::Vector2f ScreenToWorld(sf::Vector2i pixelPosition) const
    {
        return window.mapPixelToCoords(
            pixelPosition,
            camera.GetView());
    }

    sf::Vector2i WorldToScreen(sf::Vector2f worldPosition) const
    {
        return window.mapCoordsToPixel(
            worldPosition,
            camera.GetView());
    }

    void BeginWorld()
    {
        window.setView(camera.GetView());
    }

    void BeginScreen()
    {
        window.setView(window.getDefaultView());
    }

private:
    sf::RenderWindow& window;
    Camera2D camera;
};

#endif
