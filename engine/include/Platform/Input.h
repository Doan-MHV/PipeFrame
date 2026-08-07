#ifndef PIPEFRAME_INPUT_H
#define PIPEFRAME_INPUT_H

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>

#include "Platform/Key.h"
#include "Platform/MouseButton.h"

class Input
{
public:
    static void BeginFrame();
    static void HandleEvent(const sf::Event& event);

    static bool IsKeyDown(Key key);
    static bool WasKeyPressed(Key key);
    static bool WasKeyReleased(Key key);

    static bool IsMouseDown(MouseButton button);
    static bool WasMousePressed(MouseButton button);
    static bool WasMouseReleased(MouseButton button);

    static sf::Vector2i GetMousePosition();

private:
    static int ToIndex(Key key);
    static int ToIndex(MouseButton button);
};

#endif
