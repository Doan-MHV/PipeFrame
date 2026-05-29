#include "ImGuiLayer.h"

#include "Theme/EditorTheme.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

bool ImGuiLayer::Initialize(SDL_Window* window, SDL_Renderer* renderer)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    EditorTheme::Apply();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    initialized = true;
    return true;
}

void ImGuiLayer::ProcessEvent(const SDL_Event& event)
{
    if (!initialized)
    {
        return;
    }

    ImGui_ImplSDL3_ProcessEvent(&event);
}

void ImGuiLayer::BeginFrame()
{
    if (!initialized)
    {
        return;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame(SDL_Renderer* renderer)
{
    if (!initialized)
    {
        return;
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

void ImGuiLayer::Shutdown()
{
    if (!initialized)
    {
        return;
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    initialized = false;
}
