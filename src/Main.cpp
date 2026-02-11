#include "Game/Game.h"
#include "Logger/Logger.h"

int main(int argc, char* argv[])
{
    Logger::Init();

    // Log messages using the macros
    PF_INFO("Application started");
    PF_ERROR("An error occurred");
    PF_WARN("An warning occurred");

    auto game = new Game();

    game->Run();

    delete game;

    return 0;
}
