#include "PipeFrame/Core/Application.h"

#include <PipeFrame/Core/Scene.h>
#include <PipeFrame/Core/Time.h>
#include <PipeFrame/Input/Input.h>
#include <SFML/System/Clock.hpp>
#include <algorithm>
#include <cmath>

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
        activeScene->OnResize(window.getSize(), renderContext);
        activeScene->Start();
    }
}

void Application::Run() {
    sf::Clock clock;
    float fixedTimeAccumulator = 0.0f;

    while (window.isOpen()) {
        Input::BeginFrame();

        const float frameDeltaTime = std::min(clock.restart().asSeconds(), Time::MaximumFrameDeltaTime);

        while (const auto event = window.pollEvent()) {
            Input::HandleEvent(*event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto *resized = event->getIf<sf::Event::Resized>()) {
                const sf::Vector2f newSize{static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)};

                renderContext.GetCamera().SetSize(newSize);
                renderContext.SetScreenSize(resized->size);

                if (activeScene) {
                    activeScene->OnResize(resized->size, renderContext);
                }
            }

            if (activeScene) {
                activeScene->HandleEvent(*event, renderContext);
            }
        }

        fixedTimeAccumulator += frameDeltaTime;

        unsigned int fixedStepCount = 0;

        while (fixedTimeAccumulator >= Time::FixedDeltaTime && fixedStepCount < Time::MaximumFixedStepsPerFrame) {
            if (activeScene) {
                activeScene->FixedUpdate(Time::FixedDeltaTime);
            }

            fixedTimeAccumulator -= Time::FixedDeltaTime;
            ++fixedStepCount;
        }

        // The simulation cannot catch up. Keep only the fractional
        // remainder so input and rendering remain responsive.
        if (fixedTimeAccumulator >= Time::FixedDeltaTime) {
            fixedTimeAccumulator = std::fmod(fixedTimeAccumulator, Time::FixedDeltaTime);
        }

        if (activeScene) {
            activeScene->Update(frameDeltaTime);
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
