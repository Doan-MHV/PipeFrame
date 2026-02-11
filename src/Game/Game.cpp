#include "Game.h"

#include <ctime>
#include <SDL3/SDL_init.h>

#include "Logger/Logger.h"

Game::Game()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        PF_ERROR(std::format("Couldn't init SDL: {}", SDL_GetError()));
        return;
    }


    windowWidth = 800;
    windowHeight = 600;

    window = SDL_CreateWindow("", windowWidth, windowHeight,
                              SDL_WINDOW_BORDERLESS);
    if (!window)
    {
        PF_ERROR(std::format("Couldn't create window: %s\n", SDL_GetError()));
        return;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        PF_ERROR(std::format("Couldn't create renderer: %s\n", SDL_GetError()));
        return;
    }

    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    isRunning = true;
}

Game::~Game()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

glm::vec2 playerPosition;
glm::vec2 playerVelocity;

void Game::Setup()
{
    playerPosition = glm::vec2(10.0, 20.0);
    playerVelocity = glm::vec2(100.0, 0.0);
}

void Game::Run()
{
    Setup();
    while (isRunning)
    {
        ProcessInput();
        Update();
        Render();
    }
}

void Game::ProcessInput()
{
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent))
    {
        switch (sdlEvent.type)
        {
        case SDL_EVENT_QUIT:
            isRunning = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (sdlEvent.key.key == SDLK_SPACE)
            {
                isRunning = false;
            }
            break;
        default:
            break;
        }
    }
}

void Game::Update()
{
    int timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - millisecsPreviousFrame);
    if (timeToWait > 0 && timeToWait < MILLISECS_PER_FRAME)
    {
        SDL_Delay(timeToWait);
    }

    double deltaTime = (SDL_GetTicks() - millisecsPreviousFrame) / 1000.0;

    millisecsPreviousFrame = SDL_GetTicks();

    playerPosition.x += playerVelocity.x * deltaTime;
    playerPosition.y += playerVelocity.y * deltaTime;
}

void Game::Render()
{
    SDL_SetRenderDrawColor(renderer, 21, 21, 21, 255);
    SDL_RenderClear(renderer);

    SDL_Surface* surface = SDL_LoadPNG("./assets/images/tank-tiger-right.png");
    if (!surface)
    {
        SDL_Log("IMG_LoadTexture failed: %s\n", SDL_GetError());
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    SDL_FRect dstRect = {playerPosition.x, playerPosition.y, 32, 32};

    SDL_RenderTexture(renderer, texture, nullptr, &dstRect);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(renderer);
}
