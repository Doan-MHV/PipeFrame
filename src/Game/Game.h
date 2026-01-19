#ifndef PIPEFRAME_GAME_H
#define PIPEFRAME_GAME_H

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

constexpr int FPS = 60;
constexpr int MILLISECS_PER_FRAME = 1000 / FPS;

class Game
{
public:
    Game();

    ~Game();

    void Run();

private:
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
};


#endif //PIPEFRAME_GAME_H
