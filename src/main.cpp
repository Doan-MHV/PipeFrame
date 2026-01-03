#include <SDL3/SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
  // Initialize SDL
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Create a window
  SDL_Window* window = SDL_CreateWindow(
    "SDL3 Test",
    800, 600,
    SDL_WINDOW_RESIZABLE
  );
  
  if (!window) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  // Create a renderer
  SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
  if (!renderer) {
    std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  std::cout << "SDL3 initialized successfully!" << std::endl;
  std::cout << "Window created. Press ESC or close window to exit." << std::endl;

  // Main loop
  bool running = true;
  SDL_Event event;
  
  while (running) {
    // Handle events
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      } else if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_ESCAPE) {
          running = false;
        }
      }
    }

    // Clear screen with a blue color
    SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255);
    SDL_RenderClear(renderer);
    
    // Present the rendered frame
    SDL_RenderPresent(renderer);
    
    // Small delay to prevent high CPU usage
    SDL_Delay(16); // ~60 FPS
  }

  // Cleanup
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
  std::cout << "SDL3 test completed successfully!" << std::endl;
  return 0;
}
