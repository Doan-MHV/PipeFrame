#include "PipeFrame/Core/Application.h"

#include <PipeFrame/Core/Scene.h>
#include <PipeFrame/Input/Input.h>
#include <SFML/System/Clock.hpp>

Application::Application(int width, int height, const std::string &title)
    : window(sf::VideoMode({static_cast<unsigned int>(width), static_cast<unsigned int>(height)}), title),
      renderContext(window) {
    window.setFramerateLimit(60);
    renderContext.GetCamera().SetSize({static_cast<float>(width), static_cast<float>(height)});
}

void Application::SetScene(std::unique_ptr<Scene> scene) {
    if (activeScene) {
        activeScene->Stop();
        activeScene->Unload();
    }

    activeScene = std::move(scene);

    if (activeScene) {
        activeScene->Load();
        activeScene->Start();
    }
}

void Application::Run() {
    sf::Clock clock;

    while (window.isOpen()) {
        Input::BeginFrame();

        const float deltaTime = clock.restart().asSeconds();

        while (const auto event = window.pollEvent()) {
            Input::HandleEvent(*event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto *resized = event->getIf<sf::Event::Resized>()) {
                renderContext.GetCamera().SetSize(
                    {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)});
            }

            if (activeScene) {
                activeScene->HandleEvent(*event, renderContext);
            }
        }

        if (activeScene) {
            activeScene->Update(deltaTime);
        }

        window.clear(sf::Color(35, 35, 35));

        if (activeScene) {
            activeScene->Render(renderContext);
        }

        window.display();
    }

    if (activeScene) {
        activeScene->Stop();
        activeScene->Unload();
    }
}
