

#ifndef PIPEFRAME_EDITORCONTROLLER_H
#define PIPEFRAME_EDITORCONTROLLER_H

#include <string>
#include <unordered_map>

#include "imgui.h"
#include "Assets/AssetRegistry.h"
#include "ECS/ECS.h"
#include "EditorSessionState.h"
#include "EditorTypes.h"
#include "Inspector/EditorInspectorPanel.h"
#include "Layout/EditorDockSpace.h"
#include "Panels/EditorAssetPanel.h"
#include "Panels/EditorHierarchyPanel.h"
#include "Panels/EditorTilePalettePanel.h"
#include "Panels/EditorViewportPanel.h"
#include "Toolbar/EditorToolbar.h"
#include "Game/EngineMode.h"
#include "Map/TileMap.h"
#include "Prefabs/PrefabRegistry.h"
#include "Project/ProjectConfig.h"
#include "Fields/FieldGrid.h"

class ProjectModule;

class EditorController
{
    EditorSessionState state;
    EditorDockSpace dockSpace;
    EditorToolbar toolbar;
    EditorAssetPanel assetPanel;
    EditorHierarchyPanel hierarchyPanel;
    EditorTilePalettePanel tilePalettePanel;
    EditorInspectorPanel inspectorPanel;
    EditorViewportPanel viewportPanel;

public:
    EditorController() = default;

    ImVec2 GetViewportSize() const { return viewportPanel.GetViewportSize(); }
    ImVec2 GetViewportPos() const { return viewportPanel.GetViewportPos(); }
    bool IsViewportHovered() const { return viewportPanel.IsViewportHovered(); }


    EditorToolbarResult Update(EngineMode mode,
                               float playSpeed,
                               SDL_Texture* viewportTexture,
                               const std::unique_ptr<Registry>& registry,
                               const ProjectConfig& projectConfig,
                               const ComponentRegistry& componentRegistry,
                               LevelFilePaths& levelFilePaths,
                               std::unique_ptr<AssetRegistry>& assetRegistry,
                               const std::unordered_map<std::string, FieldGrid>& fieldGrids,
                               const PrefabRegistry& prefabRegistry,
                               const ClassRegistry& classRegistry,
                               ProjectModule* projectModule,
                               TileMap* tileMap,
                               SDL_Texture* tilePaletteTexture,
                               SDL_FRect& camera)
    {
        dockSpace.Draw();
        EditorToolbarResult result = toolbar.Draw(
            mode,
            playSpeed,
            state,
            registry,
            componentRegistry,
            levelFilePaths,
            tileMap
        );

        const EditorHierarchyResult hierarchyResult = hierarchyPanel.Draw(
            registry,
            state,
            prefabRegistry,
            classRegistry,
            componentRegistry,
            camera
        );
        if (hierarchyResult.requestedPrefabSave)
        {
            result.requestedPrefabSave = true;
            result.prefabSourceEntityId = hierarchyResult.prefabSourceEntityId;
            result.prefabName = hierarchyResult.prefabName;
        }

        assetPanel.Draw(*assetRegistry);

        tilePalettePanel.Draw(
            tileMap,
            tilePaletteTexture,
            state
        );

        viewportPanel.Draw(
            viewportTexture,
            registry,
            tileMap,
            fieldGrids,
            classRegistry,
            projectModule,
            camera,
            state
        );

        inspectorPanel.Draw(
            registry,
            mode,
            projectConfig,
            componentRegistry,
            tileMap,
            assetRegistry,
            fieldGrids,
            projectModule,
            state
        );

        return result;
    }
};

#endif // PIPEFRAME_EDITORCONTROLLER_H
