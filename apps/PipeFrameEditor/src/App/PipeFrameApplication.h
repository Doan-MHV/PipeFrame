#ifndef PIPEFRAME_PIPEFRAMEAPPLICATION_H
#define PIPEFRAME_PIPEFRAMEAPPLICATION_H

#include <filesystem>
#include <memory>
#include <string>

#include <SDL3/SDL.h>

#include "App/ImGuiLayer.h"
#include "EditorController.h"
#include "Game/Game.h"

class PipeFrameApplication
{
private:
    bool isRunning = false;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    ImGuiLayer imguiLayer;
    EditorController editor;
    Game game;
    void* projectModuleLibraryHandle = nullptr;
    std::filesystem::path loadedProjectModuleLibraryPath;
    bool loadedProjectModuleIsLiveCopy = false;
    unsigned long long projectModuleLiveReloadCounter = 0;

public:
    explicit PipeFrameApplication(
        const std::filesystem::path& projectFilePath = "projects/JungleDemo/PipeFrameProject.json"
    );

    bool Initialize();
    void Run();
    void Shutdown();

private:
    void ProcessInput();
    void Render();
    void CreateNewProject(
        const std::string& projectName,
        const std::string& parentDirectory,
        bool copySampleAntAssets
    );
    void OpenProject(const std::filesystem::path& projectFilePath);
    void CompileCppProject();
    void CreateCppClass(CppClassKind kind, const std::string& className);
    void CreateNewLevel(
        const std::string& levelName,
        int rows,
        int cols,
        int tileSize,
        float scale
    );
    void OpenLevel(const std::filesystem::path& levelFilePath);
    void SetStartupLevel(const std::filesystem::path& levelFilePath);
    void SaveEntityAsPrefab(int entityId, const std::string& prefabName);
    void ImportTextureAsset(const EditorToolbarResult& editorResult);
    void InstallProjectModule(const ProjectConfig& projectConfig, bool restoreCurrentWorld = false);
    void UnloadProjectModule(bool preserveCurrentWorld = false);
    std::filesystem::path GetProjectModuleLibraryPath(const ProjectConfig& projectConfig) const;
    std::filesystem::path CreateProjectModuleLiveLibraryCopy(
        const ProjectConfig& projectConfig,
        const std::filesystem::path& libraryPath
    );
    void CleanupLoadedProjectModuleCopy();
    std::string GetProjectModuleTargetName(const ProjectConfig& projectConfig) const;
};

#endif // PIPEFRAME_PIPEFRAMEAPPLICATION_H
