#include "Game/Game.h"

int main(int argc, char* argv[])
{
    auto game = new Game();

    game->Run();

    delete game;

    return 0;
}
