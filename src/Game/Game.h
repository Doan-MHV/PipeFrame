#ifndef PIPEFRAME_GAME_H
#define PIPEFRAME_GAME_H

#include <SDL3/SDL.h>

class Game {
public:
    Game();

    ~Game();

    void Run();

private:
    void ProcessInput();

    void Update();

    void Render();
};


#endif //PIPEFRAME_GAME_H
