#ifndef PIPEFRAME_BACKEND_SFML_CONVERSIONS_H
#define PIPEFRAME_BACKEND_SFML_CONVERSIONS_H

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <PipeFrame/Foundation/MathTypes.h>

namespace pipeframe::backend::sfml {

inline sf::Vector2f ToBackend(const Vector2f value) { return {value.x, value.y}; }
inline sf::Vector2i ToBackend(const Vector2i value) { return {value.x, value.y}; }
inline sf::Vector2u ToBackend(const Vector2u value) { return {value.x, value.y}; }
inline sf::FloatRect ToBackend(const Rectanglef value) { return {ToBackend(value.position), ToBackend(value.size)}; }
inline sf::IntRect ToBackend(const Rectanglei value) { return {ToBackend(value.position), ToBackend(value.size)}; }
inline sf::Color ToBackend(const Color value) { return {value.red, value.green, value.blue, value.alpha}; }

inline Vector2f FromBackend(const sf::Vector2f value) { return {value.x, value.y}; }
inline Vector2i FromBackend(const sf::Vector2i value) { return {value.x, value.y}; }
inline Vector2u FromBackend(const sf::Vector2u value) { return {value.x, value.y}; }
inline Rectanglef FromBackend(const sf::FloatRect value) { return {FromBackend(value.position), FromBackend(value.size)}; }
inline Rectanglei FromBackend(const sf::IntRect value) { return {FromBackend(value.position), FromBackend(value.size)}; }
inline Color FromBackend(const sf::Color value) { return {value.r, value.g, value.b, value.a}; }

} // namespace pipeframe::backend::sfml

#endif
