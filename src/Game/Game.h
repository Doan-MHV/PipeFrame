#ifndef PIPEFRAME_GAME_H
#define PIPEFRAME_GAME_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

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

    SDL_Window* window;
    SDL_Renderer* renderer;
};


#endif //PIPEFRAME_GAME_H
