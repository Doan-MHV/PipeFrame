
#include "PipeFrame/Render/CameraController2D.h"

#include <algorithm>
#include <cmath>

#include <PipeFrame/Input/Input.h>
#include <PipeFrame/Input/Key.h>
#include <PipeFrame/Render/Camera2D.h>
#include <PipeFrame/Render/RenderContext.h>

void CameraController2D::HandleEvent(const sf::Event &event, RenderContext &context) {
    if (const auto *pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        const bool middleMousePan = pressed->button == sf::Mouse::Button::Middle;

        const bool trackpadPan = pressed->button == sf::Mouse::Button::Left && Input::IsKeyDown(Key::Space);

        if (middleMousePan || trackpadPan) {
            dragButton = pressed->button;
            previousMousePosition = pressed->position;
        }
    }

    if (const auto *released = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (dragButton && released->button == *dragButton) {
            dragButton.reset();
        }
    }

    if (const auto *moved = event.getIf<sf::Event::MouseMoved>()) {
        if (dragButton) {
            const sf::Vector2f previousWorldPosition = context.ScreenToWorld(previousMousePosition);

            const sf::Vector2f currentWorldPosition = context.ScreenToWorld(moved->position);

            context.GetCamera().Move(previousWorldPosition - currentWorldPosition);
        }

        previousMousePosition = moved->position;
    }

    if (const auto *wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        Camera2D &camera = context.GetCamera();

        const sf::Vector2f worldBeforeZoom = context.ScreenToWorld(wheel->position);

        const float zoomMultiplier = std::pow(ZoomFactor, wheel->delta);

        const float newZoom = std::clamp(camera.GetZoom() * zoomMultiplier, MinimumZoom, MaximumZoom);

        camera.SetZoom(newZoom);

        const sf::Vector2f worldAfterZoom = context.ScreenToWorld(wheel->position);

        camera.Move(worldBeforeZoom - worldAfterZoom);
    }
}