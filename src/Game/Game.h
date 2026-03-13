#ifndef PIPEFRAME_GAME_H
#define PIPEFRAME_GAME_H

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <format>

#include "ECS/ECS.h"

constexpr int FPS = 60;
constexpr int MILLISECS_PER_FRAME = 1000 / FPS;

class Game
{
    void Setup();

    void ProcessInput();

    void Update();

    void Render();

    // NOTE: Game States
    bool isRunning = false;

    int windowWidth;
    int windowHeight;
    int millisecsPreviousFrame = 0;

    SDL_Window* window;
    SDL_Renderer* renderer;

    std::unique_ptr<Registry> registry;

public:
    Game();

    ~Game();

    void Run();
};


#endif //PIPEFRAME_GAME_H
