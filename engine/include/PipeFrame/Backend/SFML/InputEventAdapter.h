#ifndef PIPEFRAME_BACKEND_SFML_INPUT_EVENT_ADAPTER_H
#define PIPEFRAME_BACKEND_SFML_INPUT_EVENT_ADAPTER_H

#include <optional>
#include <SFML/Window/Event.hpp>

#include <PipeFrame/Input/InputEvent.h>

namespace pipeframe::backend::sfml {

InputKey FromBackend(sf::Keyboard::Key key);
sf::Keyboard::Key ToBackend(InputKey key);
PointerButton FromBackend(sf::Mouse::Button button);
sf::Mouse::Button ToBackend(PointerButton button);
std::optional<InputEvent> FromBackend(const sf::Event &event);
std::optional<sf::Event> ToBackend(const InputEvent &event);

} // namespace pipeframe::backend::sfml

#endif
