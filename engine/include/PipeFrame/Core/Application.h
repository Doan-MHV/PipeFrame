#ifndef PIPEFRAME_APPLICATION_H
#define PIPEFRAME_APPLICATION_H

#include <memory>
#include <string>

#include <SFML/Graphics/RenderWindow.hpp>

#include "PipeFrame/Core/Scene.h"
#include "PipeFrame/Render/RenderContext.h"

class Application
{
public:
    Application(int width, int height, const std::string& title);

    void SetScene(std::unique_ptr<Scene> scene);
    void Run();

private:
    sf::RenderWindow window;
    RenderContext renderContext;
    std::unique_ptr<Scene> activeScene;
};

#endif
