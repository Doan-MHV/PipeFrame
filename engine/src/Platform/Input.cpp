#include "Platform/Input.h"

#include <array>

namespace
{
    constexpr int KeyCount = 11;
    constexpr int MouseButtonCount = 4;

    std::array<bool, KeyCount> keysDown{};
    std::array<bool, KeyCount> keysPressed{};
    std::array<bool, KeyCount> keysReleased{};

    std::array<bool, MouseButtonCount> mouseDown{};
    std::array<bool, MouseButtonCount> mousePressed{};
    std::array<bool, MouseButtonCount> mouseReleased{};

    sf::Vector2i mousePosition{0, 0};

    Key FromSfmlKey(sf::Keyboard::Key key)
    {
        switch (key)
        {
        case sf::Keyboard::Key::A: return Key::A;
        case sf::Keyboard::Key::D: return Key::D;
        case sf::Keyboard::Key::S: return Key::S;
        case sf::Keyboard::Key::W: return Key::W;
        case sf::Keyboard::Key::Left: return Key::Left;
        case sf::Keyboard::Key::Right: return Key::Right;
        case sf::Keyboard::Key::Up: return Key::Up;
        case sf::Keyboard::Key::Down: return Key::Down;
        case sf::Keyboard::Key::Space: return Key::Space;
        case sf::Keyboard::Key::Escape: return Key::Escape;
        default: return Key::Unknown;
        }
    }

    MouseButton FromSfmlMouseButton(sf::Mouse::Button button)
    {
        switch (button)
        {
        case sf::Mouse::Button::Left: return MouseButton::Left;
        case sf::Mouse::Button::Right: return MouseButton::Right;
        case sf::Mouse::Button::Middle: return MouseButton::Middle;
        default: return MouseButton::Unknown;
        }
    }
}

void Input::BeginFrame()
{
    keysPressed.fill(false);
    keysReleased.fill(false);

    mousePressed.fill(false);
    mouseReleased.fill(false);
}

void Input::HandleEvent(const sf::Event& event)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        const Key key = FromSfmlKey(keyPressed->code);
        const int index = ToIndex(key);

        if (index >= 0)
        {
            keysDown[index] = true;
            keysPressed[index] = true;
        }
    }

    if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>())
    {
        const Key key = FromSfmlKey(keyReleased->code);
        const int index = ToIndex(key);

        if (index >= 0)
        {
            keysDown[index] = false;
            keysReleased[index] = true;
        }
    }

    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        mousePosition = mouseMoved->position;
    }

    if (const auto* buttonPressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        const MouseButton button = FromSfmlMouseButton(buttonPressed->button);
        const int index = ToIndex(button);

        if (index >= 0)
        {
            mouseDown[index] = true;
            mousePressed[index] = true;
        }

        mousePosition = buttonPressed->position;
    }

    if (const auto* buttonReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        const MouseButton button = FromSfmlMouseButton(buttonReleased->button);
        const int index = ToIndex(button);

        if (index >= 0)
        {
            mouseDown[index] = false;
            mouseReleased[index] = true;
        }

        mousePosition = buttonReleased->position;
    }
}

bool Input::IsKeyDown(Key key)
{
    const int index = ToIndex(key);
    return index >= 0 && keysDown[index];
}

bool Input::WasKeyPressed(Key key)
{
    const int index = ToIndex(key);
    return index >= 0 && keysPressed[index];
}

bool Input::WasKeyReleased(Key key)
{
    const int index = ToIndex(key);
    return index >= 0 && keysReleased[index];
}

bool Input::IsMouseDown(MouseButton button)
{
    const int index = ToIndex(button);
    return index >= 0 && mouseDown[index];
}

bool Input::WasMousePressed(MouseButton button)
{
    const int index = ToIndex(button);
    return index >= 0 && mousePressed[index];
}

bool Input::WasMouseReleased(MouseButton button)
{
    const int index = ToIndex(button);
    return index >= 0 && mouseReleased[index];
}

sf::Vector2i Input::GetMousePosition()
{
    return mousePosition;
}

int Input::ToIndex(Key key)
{
    return static_cast<int>(key);
}

int Input::ToIndex(MouseButton button)
{
    return static_cast<int>(button);
}
