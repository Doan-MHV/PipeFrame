#include <PipeFrame/Backend/SFML/InputEventAdapter.h>

namespace pipeframe::backend::sfml {

InputKey FromBackend(const sf::Keyboard::Key key) {
#define PF_KEY(name) case sf::Keyboard::Key::name: return InputKey::name
    switch (key) {
        PF_KEY(A); PF_KEY(B); PF_KEY(D); PF_KEY(F); PF_KEY(H); PF_KEY(L); PF_KEY(N); PF_KEY(P); PF_KEY(R); PF_KEY(S); PF_KEY(U); PF_KEY(W);
        PF_KEY(Num1); PF_KEY(Num2); PF_KEY(Num3); PF_KEY(Num4); PF_KEY(Left); PF_KEY(Right); PF_KEY(Up); PF_KEY(Down);
        PF_KEY(Space); PF_KEY(Period); PF_KEY(Escape); PF_KEY(Enter); PF_KEY(Tab); PF_KEY(Delete); PF_KEY(Home); PF_KEY(End); PF_KEY(PageUp); PF_KEY(PageDown); PF_KEY(E); PF_KEY(Z); PF_KEY(F6); PF_KEY(F7); PF_KEY(F8); PF_KEY(Backspace);
        default: return InputKey::Unknown;
    }
#undef PF_KEY
}

sf::Keyboard::Key ToBackend(const InputKey key) {
#define PF_KEY(name) case InputKey::name: return sf::Keyboard::Key::name
    switch (key) {
        PF_KEY(A); PF_KEY(B); PF_KEY(D); PF_KEY(F); PF_KEY(H); PF_KEY(L); PF_KEY(N); PF_KEY(P); PF_KEY(R); PF_KEY(S); PF_KEY(U); PF_KEY(W);
        PF_KEY(Num1); PF_KEY(Num2); PF_KEY(Num3); PF_KEY(Num4); PF_KEY(Left); PF_KEY(Right); PF_KEY(Up); PF_KEY(Down);
        PF_KEY(Space); PF_KEY(Period); PF_KEY(Escape); PF_KEY(Enter); PF_KEY(Tab); PF_KEY(Delete); PF_KEY(Home); PF_KEY(End); PF_KEY(PageUp); PF_KEY(PageDown); PF_KEY(E); PF_KEY(Z); PF_KEY(F6); PF_KEY(F7); PF_KEY(F8); PF_KEY(Backspace);
        default: return sf::Keyboard::Key::Unknown;
    }
#undef PF_KEY
}

PointerButton FromBackend(const sf::Mouse::Button button) {
    switch (button) {
        case sf::Mouse::Button::Left: return PointerButton::Left;
        case sf::Mouse::Button::Right: return PointerButton::Right;
        case sf::Mouse::Button::Middle: return PointerButton::Middle;
        default: return PointerButton::Unknown;
    }
}

sf::Mouse::Button ToBackend(const PointerButton button) {
    switch (button) {
        case PointerButton::Left: return sf::Mouse::Button::Left;
        case PointerButton::Right: return sf::Mouse::Button::Right;
        case PointerButton::Middle: return sf::Mouse::Button::Middle;
        default: return sf::Mouse::Button::Left;
    }
}

std::optional<InputEvent> FromBackend(const sf::Event &event) {
    if (const auto *value = event.getIf<sf::Event::KeyPressed>())
        return InputEvent{InputEventType::KeyPressed, KeyInput{FromBackend(value->code), value->alt, value->control, value->shift, value->system}};
    if (const auto *value = event.getIf<sf::Event::KeyReleased>())
        return InputEvent{InputEventType::KeyReleased, KeyInput{FromBackend(value->code), value->alt, value->control, value->shift, value->system}};
    if (const auto *value = event.getIf<sf::Event::MouseButtonPressed>())
        return InputEvent{InputEventType::PointerPressed, PointerInput{FromBackend(value->button), {value->position.x, value->position.y}}};
    if (const auto *value = event.getIf<sf::Event::MouseButtonReleased>())
        return InputEvent{InputEventType::PointerReleased, PointerInput{FromBackend(value->button), {value->position.x, value->position.y}}};
    if (const auto *value = event.getIf<sf::Event::MouseMoved>())
        return InputEvent{InputEventType::PointerMoved, PointerMoveInput{{value->position.x, value->position.y}}};
    if (const auto *value = event.getIf<sf::Event::MouseWheelScrolled>())
        return InputEvent{InputEventType::WheelScrolled, WheelInput{value->delta, {value->position.x, value->position.y}, value->wheel == sf::Mouse::Wheel::Horizontal}};
    if (const auto *value = event.getIf<sf::Event::TextEntered>())
        return InputEvent{InputEventType::TextEntered, TextInput{value->unicode}};
    if (event.is<sf::Event::FocusGained>()) return InputEvent{InputEventType::FocusChanged, FocusInput{true}};
    if (event.is<sf::Event::FocusLost>()) return InputEvent{InputEventType::FocusChanged, FocusInput{false}};
    if (event.is<sf::Event::MouseEntered>()) return InputEvent{InputEventType::PointerPresenceChanged, PointerPresenceInput{true}};
    if (event.is<sf::Event::MouseLeft>()) return InputEvent{InputEventType::PointerPresenceChanged, PointerPresenceInput{false}};
    return std::nullopt;
}

std::optional<sf::Event> ToBackend(const InputEvent &event) {
    if (const auto *value = event.GetIf<KeyInput>()) {
        if (event.type == InputEventType::KeyPressed) return sf::Event::KeyPressed{ToBackend(value->key), sf::Keyboard::Scancode::Unknown, value->alt, value->control, value->shift, value->system};
        if (event.type == InputEventType::KeyReleased) return sf::Event::KeyReleased{ToBackend(value->key), sf::Keyboard::Scancode::Unknown, value->alt, value->control, value->shift, value->system};
    }
    if (const auto *value = event.GetIf<PointerInput>()) {
        const sf::Vector2i position{value->position.x, value->position.y};
        if (event.type == InputEventType::PointerPressed) return sf::Event::MouseButtonPressed{ToBackend(value->button), position};
        if (event.type == InputEventType::PointerReleased) return sf::Event::MouseButtonReleased{ToBackend(value->button), position};
    }
    if (const auto *value = event.GetIf<PointerMoveInput>()) return sf::Event::MouseMoved{{value->position.x, value->position.y}};
    if (const auto *value = event.GetIf<WheelInput>()) return sf::Event::MouseWheelScrolled{value->horizontal ? sf::Mouse::Wheel::Horizontal : sf::Mouse::Wheel::Vertical, value->delta, {value->position.x, value->position.y}};
    if (const auto *value = event.GetIf<TextInput>()) return sf::Event::TextEntered{value->unicode};
    if (const auto *value = event.GetIf<FocusInput>()) return value->focused ? sf::Event{sf::Event::FocusGained{}} : sf::Event{sf::Event::FocusLost{}};
    if (const auto *value = event.GetIf<PointerPresenceInput>()) return value->inside ? sf::Event{sf::Event::MouseEntered{}} : sf::Event{sf::Event::MouseLeft{}};
    return std::nullopt;
}

} // namespace pipeframe::backend::sfml
