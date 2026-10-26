#ifndef PIPEFRAME_INPUT_EVENT_H
#define PIPEFRAME_INPUT_EVENT_H

#include <PipeFrame/Foundation/MathTypes.h>

#include <cstdint>
#include <variant>

namespace pipeframe {

enum class InputKey : std::uint16_t {
    Unknown,
    A,
    B,
    D,
    F,
    H,
    L,
    N,
    P,
    R,
    S,
    U,
    W,
    Num1,
    Num2,
    Num3,
    Num4,
    Left,
    Right,
    Up,
    Down,
    Space,
    Period,
    Escape,
    Enter,
    Tab,
    Delete,
    Home,
    End,
    PageUp,
    PageDown,
    E,
    Z,
    F6,
    F7,
    F8,
    Backspace
};

enum class PointerButton : std::uint8_t { Unknown, Left, Right, Middle };

struct KeyInput {
    InputKey key{InputKey::Unknown};
    bool alt{};
    bool control{};
    bool shift{};
    bool system{};
};
struct PointerInput {
    PointerButton button{PointerButton::Unknown};
    Vector2i position{};
};
struct PointerMoveInput {
    Vector2i position{};
};
struct WheelInput {
    float delta{};
    Vector2i position{};
    bool horizontal{};
};
struct TextInput {
    char32_t unicode{};
};
struct FocusInput {
    bool focused{};
};
struct PointerPresenceInput {
    bool inside{};
};

using InputEventPayload =
    std::variant<KeyInput, PointerInput, PointerMoveInput, WheelInput, TextInput, FocusInput, PointerPresenceInput>;

enum class InputEventType : std::uint8_t {
    KeyPressed,
    KeyReleased,
    PointerPressed,
    PointerReleased,
    PointerMoved,
    WheelScrolled,
    TextEntered,
    FocusChanged,
    PointerPresenceChanged
};

struct InputEvent {
    InputEventType type{InputEventType::FocusChanged};
    InputEventPayload payload{FocusInput{}};

    template <typename T>
    [[nodiscard]] const T* GetIf() const {
        return std::get_if<T>(&payload);
    }
};

}  // namespace pipeframe

#endif
