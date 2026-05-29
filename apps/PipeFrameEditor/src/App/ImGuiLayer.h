#ifndef PIPEFRAME_IMGUILAYER_H
#define PIPEFRAME_IMGUILAYER_H

#include <SDL3/SDL.h>

class ImGuiLayer
{
private:
    bool initialized = false;

public:
    bool Initialize(SDL_Window* window, SDL_Renderer* renderer);
    void ProcessEvent(const SDL_Event& event);
    void BeginFrame();
    void EndFrame(SDL_Renderer* renderer);
    void Shutdown();
};

#endif // PIPEFRAME_IMGUILAYER_H
