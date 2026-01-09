#include "Game/Game.h"

int main(int argc, char* argv[]) {
    Game *game = new Game();

    game->Run();

    delete game;

    return 0;
}
