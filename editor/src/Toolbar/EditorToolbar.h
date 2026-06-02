

#ifndef PIPEFRAME_EDITORTOOLBAR_H
#define PIPEFRAME_EDITORTOOLBAR_H


#include <memory>
#include <string>

#include "Core/EditorCommands.h"
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
