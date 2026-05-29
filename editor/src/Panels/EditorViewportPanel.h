#ifndef PIPEFRAME_EDITORVIEWPORTPANEL_H
#define PIPEFRAME_EDITORVIEWPORTPANEL_H

#include <memory>
#include <string>
#include <unordered_map>

#include <SDL3/SDL.h>

#include "EditorSessionState.h"
#include "imgui.h"

#include "EditorTypes.h"
#include "Map/TileMap.h"
#include "Reflection/EditorMetadata.h"
#include "Fields/FieldGrid.h"

class Entity;
class ProjectModule;
class Registry;

class EditorViewportPanel
{
private:
    ImVec2 viewportSize = ImVec2(0.0f, 0.0f);
    ImVec2 viewportPos = ImVec2(0.0f, 0.0f);
    bool viewportHovered = false;

    bool isDraggingEntity = false;
    float dragOffsetX = 0.0f;
    float dragOffsetY = 0.0f;

    int hoveredTileRow = -1;
    int hoveredTileCol = -1;

    int lastPaintedTileRow = -1;
    int lastPaintedTileCol = -1;

    bool isPanningViewport = false;
    ImVec2 lastPanMousePos = ImVec2(0.0f, 0.0f);

public:
    void Draw(
        SDL_Texture* viewportTexture,
        const std::unique_ptr<Registry>& registry,
        TileMap* tileMap,
        const std::unordered_map<std::string, FieldGrid>& fieldGrids,
        const ClassRegistry& classRegistry,
        ProjectModule* projectModule,
        SDL_FRect& camera,
        EditorSessionState& state
    );

    ImVec2 GetViewportSize() const { return viewportSize; }
    ImVec2 GetViewportPos() const { return viewportPos; }
    bool IsViewportHovered() const { return viewportHovered; }

private:
    void ClampCamera(SDL_FRect& camera, const TileMap* tileMap);
    void HandleViewportPanning(SDL_FRect& camera, const TileMap* tileMap, const ImVec2& mousePos);
    void UpdateHoveredTile(const TileMap* tileMap, const SDL_FRect& camera, const ImVec2& mousePos);

    void DrawHoveredTileOutline(const TileMap* tileMap, const SDL_FRect& camera);
    void DrawSelectedTileOutline(
        const TileMap* tileMap,
        const SDL_FRect& camera,
        bool hasSelectedTile,
        int selectedTileRow,
        int selectedTileCol
    );
    void DrawSelectedEntityOutline(
        const std::unique_ptr<Registry>& registry,
        const SDL_FRect& camera,
        int selectedEntityId
    );
    void DrawTerrainOverlay(
        const TileMap* tileMap,
        const SDL_FRect& camera,
        bool showTerrainOverlay
    );
    void DrawFieldOverlay(
        const std::unordered_map<std::string, FieldGrid>& fieldGrids,
        const SDL_FRect& camera,
        bool showFieldOverlay
    );
    void DrawColliderOverlay(
        const std::unique_ptr<Registry>& registry,
        const SDL_FRect& camera,
        bool showColliderOverlay
    );
    void HandleTileSelection(
        const TileMap* tileMap,
        const SDL_FRect& camera,
        const ImVec2& mousePos,
        bool& hasSelectedTile,
        int& selectedTileRow,
        int& selectedTileCol,
        int& selectedEntityId
    );

    void HandleTilePainting(
        TileMap* tileMap,
        const SDL_FRect& camera,
        const ImVec2& mousePos,
        int brushTileRow,
        int brushTileCol,
        bool& hasSelectedTile,
        int& selectedTileRow,
        int& selectedTileCol
    );

    void HandleTerrainPainting(
        TileMap* tileMap,
        const SDL_FRect& camera,
        const ImVec2& mousePos,
        TerrainType terrainBrush,
        bool& hasSelectedTile,
        int& selectedTileRow,
        int& selectedTileCol
    );

    void HandleViewportSelectAndDragging(
        const std::unique_ptr<Registry>& registry,
        ProjectModule* projectModule,
        const SDL_FRect& camera,
        const ImVec2& mousePos,
        EditorSessionState& state
    );

    void HandleProjectClassDrop(
        const std::unique_ptr<Registry>& registry,
        const ClassRegistry& classRegistry,
        const SDL_FRect& camera,
        const ImVec2& mousePos,
        EditorSessionState& state
    );

    int PickEntityAtWorldPosition(
        const std::unique_ptr<Registry>& registry,
        float worldX,
        float worldY
    );

    Entity FindEntityById(
        const std::unique_ptr<Registry>& registry,
        int entityId
    );

    ImU32 GetTerrainOverlayColor(TerrainType terrain) const;
    ImU32 GetFieldOverlayColor(const std::string& fieldName, double value, double maxValue) const;
};

#endif // PIPEFRAME_EDITORVIEWPORTPANEL_H
