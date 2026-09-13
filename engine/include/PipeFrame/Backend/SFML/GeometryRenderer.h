#ifndef PIPEFRAME_BACKEND_SFML_GEOMETRY_RENDERER_H
#define PIPEFRAME_BACKEND_SFML_GEOMETRY_RENDERER_H
#include <PipeFrame/Render/RenderTypes.h>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
namespace pipeframe::backend::sfml {void DrawGeometry(sf::RenderTarget &target,const GeometryCommand &command,const sf::RenderStates &states=sf::RenderStates::Default);}
#endif
