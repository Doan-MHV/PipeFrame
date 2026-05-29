

#ifndef PIPEFRAME_EDITORTOOLBAR_H
#define PIPEFRAME_EDITORTOOLBAR_H


#include <memory>
#include <string>

#include "Dialogs/CreateCppClassDialog.h"
#include "EditorSessionState.h"
#include "Dialogs/NewLevelDialog.h"
#include "Dialogs/NewProjectDialog.h"
#include "Dialogs/OpenProjectDialog.h"
#include "Dialogs/TextureImportDialog.h"
#include "Game/EngineMode.h"
#include "Map/TileMap.h"
#include "Toolbar/EditorSaveSection.h"

class Registry;
class ComponentRegistry;

struct EditorToolbarResult
{
    bool requestedModeToggle = false;
    bool requestedProjectCreate = false;
    bool requestedProjectOpen = false;
    bool requestedLevelCreate = false;
    bool requestedCppCompile = false;
    bool requestedCppClassCreate = false;
    bool requestedPrefabSave = false;
    bool requestedTextureImport = false;
    bool requestedPlaySpeedChange = false;
    int prefabSourceEntityId = -1;
    float playSpeed = 1.0f;
    std::string projectName;
    std::string projectParentDirectory;
    std::string projectFilePath;
    std::string prefabName;
    std::string textureAssetId;
    std::string textureSourceFilePath;
    TextureImportMode textureImportMode = TextureImportMode::SingleImage;
    int textureDisplayWidth = 32;
    int textureDisplayHeight = 32;
    int textureFrameWidth = 32;
    int textureFrameHeight = 32;
    int textureDefaultFrame = 0;
    bool copySampleAntAssets = false;
    CppClassKind cppClassKind = CppClassKind::Component;
    std::string cppClassName;
    std::string levelName;
    int levelRows = 16;
    int levelCols = 16;
    int levelTileSize = 32;
    float levelScale = 2.0f;
};

class EditorToolbar
{
private:
    NewProjectDialog newProjectDialog;
    OpenProjectDialog openProjectDialog;
    NewLevelDialog newLevelDialog;
    CreateCppClassDialog createCppClassDialog;
    TextureImportDialog textureImportDialog;
    EditorSaveSection saveSection;

public:
    EditorToolbarResult Draw(
        EngineMode mode,
        float playSpeed,
        EditorSessionState& state,
        const std::unique_ptr<Registry>& registry,
        const ComponentRegistry& componentRegistry,
        LevelFilePaths& levelFilePaths,
        TileMap* tileMap
    );

private:
};


#endif //PIPEFRAME_EDITORTOOLBAR_H
