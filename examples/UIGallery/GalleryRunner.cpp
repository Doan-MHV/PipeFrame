// Standalone native runner only. Project UI lives in Main.cpp.
#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include <PipeFrame/Project/ProjectRuntime.h>

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <iostream>
std::unique_ptr<pipeframe::ProjectRuntime> CreateGalleryProject();
int main(int argc, char** argv) {
    auto project = CreateGalleryProject();
    std::string error;
    if (!project->Load({}, error)) {
        std::cerr << error;
        return 1;
    }
    if (argc > 1 && (std::string(argv[1]) == "--check" || std::string(argv[1]) == "--snapshot")) {
        sf::RenderTexture target({800, 600});
        auto context = pipeframe::backend::sfml::MakeRenderContext(target);
        target.clear({24, 25, 28});
        for (int i = 0; i < 3; ++i)
            project->RenderScreen(context);
        target.display();
        if (argc > 2 && std::string(argv[1]) == "--snapshot" && !target.getTexture().copyToImage().saveToFile(argv[2]))
            return 1;
        project->Unload();
        return 0;
    }
    sf::RenderWindow window(sf::VideoMode({1100, 800}), "PipeFrame Declarative UI Gallery");
    auto context = pipeframe::backend::sfml::MakeRenderContext(window);
    window.setFramerateLimit(60);
    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* resize = event->getIf<sf::Event::Resized>())
                context.SetScreenSize({resize->size.x, resize->size.y});
            if (auto input = pipeframe::backend::sfml::FromBackend(*event)) project->HandleUIEvent(*input, context);
        }
        if (!window.isOpen()) break;
        window.clear({24, 25, 28});
        project->RenderScreen(context);
        window.display();
    }
    project->Unload();
}
