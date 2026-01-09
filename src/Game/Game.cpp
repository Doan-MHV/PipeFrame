#include "Game.h"

Game::Game() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't init sdl: %s\n", SDL_GetError());
        return;
    }

    SDL_Window *window = SDL_CreateWindow("", 800, 600,
                                          SDL_WINDOW_BORDERLESS);
    if (!window) {
        SDL_Log("Couldn't create window: %s\n", SDL_GetError());
        return;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Couldn't create renderer: %s\n", SDL_GetError());
        return;
    }
}

Game::~Game() {
}

void Game::Run() {
}

void Game::ProcessInput() {
}

void Game::Update() {
}

void Game::Render() {
}

