#include "EditorViewportPanel.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "imgui.h"
#include "Collision/BoxColliderGeometry.h"
#include "Components/BoxColliderComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Simulation/ProjectModule.h"

namespace
{
void DrawWorldRectangle(
    ImDrawList* drawList,
    const std::array<glm::vec2, 4>& corners,
    const ImVec2& viewportPos,
    const SDL_FRect& camera,
    ImU32 color,
    float thickness
)
{
    ImVec2 points[5];
    for (std::size_t i = 0; i < corners.size(); i++)
    {
        points[i] = ImVec2(
            viewportPos.x + corners[i].x - camera.x,
            viewportPos.y + corners[i].y - camera.y
        );
    }
    points[4] = points[0];
    drawList->AddPolyline(points, 5, color, 0, thickness);
}
}

void EditorViewportPanel::ClampCamera(SDL_FRect& camera, const TileMap* tileMap)
{
    if (!tileMap)
    {
        return;
    }

    float maxCameraX = static_cast<float>(tileMap->GetWorldWidth()) - camera.w;
    float maxCameraY = static_cast<float>(tileMap->GetWorldHeight()) - camera.h;

    if (maxCameraX < 0.0f)
    {
        maxCameraX = 0.0f;
    }

    if (maxCameraY < 0.0f)
    {
        maxCameraY = 0.0f;
    }

    if (camera.x < 0.0f)
    {
        camera.x = 0.0f;
    }

    if (camera.y < 0.0f)
    {
        camera.y = 0.0f;
    }

    if (camera.x > maxCameraX)
    {
        camera.x = maxCameraX;
    }

    if (camera.y > maxCameraY)
    {
        camera.y = maxCameraY;
    }
}

void EditorViewportPanel::HandleViewportPanning(
    SDL_FRect& camera,
    const TileMap* tileMap,
    const ImVec2& mousePos
)
{
    if (viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        isPanningViewport = true;
        lastPanMousePos = mousePos;
    }

    if (isPanningViewport && ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        ImVec2 delta = ImVec2(
            mousePos.x - lastPanMousePos.x,
            mousePos.y - lastPanMousePos.y
        );

        camera.x -= delta.x;
        camera.y -= delta.y;

        ClampCamera(camera, tileMap);

        lastPanMousePos = mousePos;
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        isPanningViewport = false;
    }
}

void EditorViewportPanel::UpdateHoveredTile(
    const TileMap* tileMap,
    const SDL_FRect& camera,
    const ImVec2& mousePos
)
{
    hoveredTileRow = -1;
    hoveredTileCol = -1;

    if (!tileMap || !viewportHovered)
    {
        return;
    }

    float localX = mousePos.x - viewportPos.x;
    float localY = mousePos.y - viewportPos.y;

    float worldX = camera.x + localX;
    float worldY = camera.y + localY;

    int row = 0;
    int col = 0;

    if (tileMap->WorldToGrid(worldX, worldY, row, col))
    {
        hoveredTileRow = row;
        hoveredTileCol = col;
    }
}

void EditorViewportPanel::DrawHoveredTileOutline(const TileMap* tileMap, const SDL_FRect& camera)
{
    if (!tileMap || hoveredTileRow < 0 || hoveredTileCol < 0)
    {
        return;
    }

    const float tileSize = tileMap->GetTileSize() * tileMap->GetScale();

    float worldX = hoveredTileCol * tileSize;
    float worldY = hoveredTileRow * tileSize;

    float screenX = viewportPos.x + (worldX - camera.x);
    float screenY = viewportPos.y + (worldY - camera.y);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRect(
        ImVec2(screenX, screenY),
        ImVec2(screenX + tileSize, screenY + tileSize),
        IM_COL32(255, 255, 255, 180),
        0.0f,
        0,
        1.5f
    );
}

void EditorViewportPanel::DrawSelectedTileOutline(
    const TileMap* tileMap,
    const SDL_FRect& camera,
    bool hasSelectedTile,
    int selectedTileRow,
    int selectedTileCol
)
{
    if (!tileMap || !hasSelectedTile)
    {
        return;
    }

    const float tileSize = tileMap->GetTileSize() * tileMap->GetScale();

    float worldX = selectedTileCol * tileSize;
    float worldY = selectedTileRow * tileSize;

    float screenX = viewportPos.x + (worldX - camera.x);
    float screenY = viewportPos.y + (worldY - camera.y);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRect(
        ImVec2(screenX, screenY),
        ImVec2(screenX + tileSize, screenY + tileSize),
        IM_COL32(255, 255, 0, 255),
        0.0f,
        0,
        2.0f
    );
}

Entity EditorViewportPanel::FindEntityById(
    const std::unique_ptr<Registry>& registry,
    int entityId
)
{
    for (auto entity : registry->GetAllEntities())
    {
        if (entity.GetId() == entityId)
        {
            return entity;
        }
    }

    return Entity(-1);
}

void EditorViewportPanel::DrawSelectedEntityOutline(
    const std::unique_ptr<Registry>& registry,
    const SDL_FRect& camera,
    int selectedEntityId
)
{
    if (selectedEntityId < 0)
    {
        return;
    }

    Entity selectedEntity = FindEntityById(registry, selectedEntityId);
    if (selectedEntity.GetId() < 0 || !selectedEntity.HasComponent<TransformComponent>())
    {
        return;
    }

    const auto& transform = selectedEntity.GetComponent<TransformComponent>();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (selectedEntity.HasComponent<SpriteComponent>())
    {
        const auto& sprite = selectedEntity.GetComponent<SpriteComponent>();

        const SDL_FRect spriteCamera = sprite.isFixed
            ? SDL_FRect{0.0f, 0.0f, camera.w, camera.h}
            : camera;

        BoxColliderGeometry spriteGeometry;
        spriteGeometry.x = transform.position.x;
        spriteGeometry.y = transform.position.y;
        spriteGeometry.width = std::max(1.0f, static_cast<float>(sprite.width) * transform.scale.x);
        spriteGeometry.height = std::max(1.0f, static_cast<float>(sprite.height) * transform.scale.y);
        spriteGeometry.rotationDegrees = static_cast<float>(transform.rotation);
        spriteGeometry.rotated = std::abs(spriteGeometry.rotationDegrees) > 0.001f;

        DrawWorldRectangle(
            drawList,
            GetBoxColliderCorners(spriteGeometry),
            viewportPos,
            spriteCamera,
            IM_COL32(255, 225, 80, 255),
            2.0f
        );
        return;
    }

    if (selectedEntity.HasComponent<BoxColliderComponent>())
    {
        DrawWorldRectangle(
            drawList,
            GetBoxColliderCorners(GetBoxColliderGeometry(selectedEntity)),
            viewportPos,
            camera,
            IM_COL32(255, 225, 80, 255),
            2.0f
        );
    }
}

void EditorViewportPanel::DrawWorldBounds(const TileMap* tileMap, const SDL_FRect& camera)
{
    if (!tileMap)
    {
        return;
    }

    const float screenLeft = viewportPos.x - camera.x;
    const float screenTop = viewportPos.y - camera.y;
    const float screenRight = screenLeft + static_cast<float>(tileMap->GetWorldWidth());
    const float screenBottom = screenTop + static_cast<float>(tileMap->GetWorldHeight());

    const ImVec2 clipMin = viewportPos;
    const ImVec2 clipMax(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(clipMin, clipMax, true);
    drawList->AddRect(
        ImVec2(screenLeft, screenTop),
        ImVec2(screenRight, screenBottom),
        IM_COL32(120, 190, 190, 190),
        0.0f,
        0,
        2.0f
    );
    drawList->PopClipRect();
}

ImU32 EditorViewportPanel::GetTerrainOverlayColor(TerrainType terrain) const
{
    switch (terrain)
    {
    case TerrainType::Land:
        return IM_COL32(0, 255, 0, 60);

    case TerrainType::Water:
        return IM_COL32(0, 120, 255, 90);

    case TerrainType::Blocked:
        return IM_COL32(255, 0, 0, 90);

    case TerrainType::Runway:
        return IM_COL32(255, 255, 0, 80);
    }

    return IM_COL32(255, 255, 255, 40);
}

void EditorViewportPanel::DrawTerrainOverlay(
    const TileMap* tileMap,
    const SDL_FRect& camera,
    bool showTerrainOverlay
)
{
    if (!tileMap || !showTerrainOverlay)
    {
        return;
    }

    const float tileSize = tileMap->GetTileSize() * tileMap->GetScale();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (int row = 0; row < tileMap->GetRows(); row++)
    {
        for (int col = 0; col < tileMap->GetCols(); col++)
        {
            const TileCell& tile = tileMap->GetTile(row, col);

            float worldX = col * tileSize;
            float worldY = row * tileSize;

            float screenX = viewportPos.x + (worldX - camera.x);
            float screenY = viewportPos.y + (worldY - camera.y);

            if (screenX + tileSize < viewportPos.x ||
                screenX > viewportPos.x + viewportSize.x ||
                screenY + tileSize < viewportPos.y ||
                screenY > viewportPos.y + viewportSize.y)
            {
                continue;
            }

            drawList->AddRectFilled(
                ImVec2(screenX, screenY),
                ImVec2(screenX + tileSize, screenY + tileSize),
                GetTerrainOverlayColor(tile.terrain)
            );
        }
    }
}

ImU32 EditorViewportPanel::GetFieldOverlayColor(
    const std::string& fieldName,
    double value,
    double maxValue
) const
{
    if (maxValue <= 0.0 || value <= 0.0)
    {
        return IM_COL32(0, 0, 0, 0);
    }

    const float intensity = std::clamp(static_cast<float>(value / maxValue), 0.0f, 1.0f);
    const int alpha = static_cast<int>(35.0f + 145.0f * intensity);

    if (fieldName == "food")
    {
        return IM_COL32(255, 185, 40, alpha);
    }

    if (fieldName == "home")
    {
        return IM_COL32(60, 180, 255, alpha);
    }

    return IM_COL32(210, 90, 255, alpha);
}

void EditorViewportPanel::DrawFieldOverlay(
    const std::unordered_map<std::string, FieldGrid>& fieldGrids,
    const SDL_FRect& camera,
    bool showFieldOverlay
)
{
    if (!showFieldOverlay || fieldGrids.empty())
    {
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (const auto& [fieldName, fieldGrid] : fieldGrids)
    {
        if (fieldGrid.rows <= 0 ||
            fieldGrid.cols <= 0 ||
            fieldGrid.cellWorldSize <= 0.0f ||
            fieldGrid.values.empty())
        {
            continue;
        }

        const int startCol = std::max(
            static_cast<int>(std::floor(camera.x / fieldGrid.cellWorldSize)),
            0
        );
        const int endCol = std::min(
            static_cast<int>(std::ceil((camera.x + viewportSize.x) / fieldGrid.cellWorldSize)),
            fieldGrid.cols - 1
        );
        const int startRow = std::max(
            static_cast<int>(std::floor(camera.y / fieldGrid.cellWorldSize)),
            0
        );
        const int endRow = std::min(
            static_cast<int>(std::ceil((camera.y + viewportSize.y) / fieldGrid.cellWorldSize)),
            fieldGrid.rows - 1
        );

        if (startCol > endCol || startRow > endRow)
        {
            continue;
        }

        double maxValue = 0.0;
        for (int row = startRow; row <= endRow; row++)
        {
            for (int col = startCol; col <= endCol; col++)
            {
                const std::size_t index = static_cast<std::size_t>(row) *
                    static_cast<std::size_t>(fieldGrid.cols) +
                    static_cast<std::size_t>(col);

                if (index < fieldGrid.values.size())
                {
                    maxValue = std::max(maxValue, fieldGrid.values[index]);
                }
            }
        }

        if (maxValue <= 0.0)
        {
            continue;
        }

        for (int row = startRow; row <= endRow; row++)
        {
            for (int col = startCol; col <= endCol; col++)
            {
                const std::size_t index = static_cast<std::size_t>(row) *
                    static_cast<std::size_t>(fieldGrid.cols) +
                    static_cast<std::size_t>(col);

                if (index >= fieldGrid.values.size())
                {
                    continue;
                }

                const double value = fieldGrid.values[index];
                if (value <= 0.001)
                {
                    continue;
                }

                const float worldX = col * fieldGrid.cellWorldSize;
                const float worldY = row * fieldGrid.cellWorldSize;
                const float screenX = viewportPos.x + (worldX - camera.x);
                const float screenY = viewportPos.y + (worldY - camera.y);

                drawList->AddRectFilled(
                    ImVec2(screenX, screenY),
                    ImVec2(screenX + fieldGrid.cellWorldSize, screenY + fieldGrid.cellWorldSize),
                    GetFieldOverlayColor(fieldName, value, maxValue)
                );
            }
        }
    }
}

void EditorViewportPanel::DrawColliderOverlay(
    const std::unique_ptr<Registry>& registry,
    const SDL_FRect& camera,
    bool showColliderOverlay
)
{
    if (!showColliderOverlay)
    {
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (auto entity : registry->GetAllEntities())
    {
        if (!entity.HasComponent<TransformComponent>() ||
            !entity.HasComponent<BoxColliderComponent>())
        {
            continue;
        }

        DrawWorldRectangle(
            drawList,
            GetBoxColliderCorners(GetBoxColliderGeometry(entity)),
            viewportPos,
            camera,
            IM_COL32(255, 95, 95, 190),
            1.5f
        );
    }
}

void EditorViewportPanel::HandleTileSelection(
    const TileMap* tileMap,
    const SDL_FRect& camera,
    const ImVec2& mousePos,
    bool& hasSelectedTile,
    int& selectedTileRow,
    int& selectedTileCol,
    int& selectedEntityId
)
{
    if (!tileMap)
    {
        return;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        return;
    }

    float localX = mousePos.x - viewportPos.x;
    float localY = mousePos.y - viewportPos.y;

    float worldX = camera.x + localX;
    float worldY = camera.y + localY;

    int row = 0;
    int col = 0;

    if (tileMap->WorldToGrid(worldX, worldY, row, col))
    {
        hasSelectedTile = true;
        selectedTileRow = row;
        selectedTileCol = col;

        selectedEntityId = -1;
        isDraggingEntity = false;
    }
    else
    {
        hasSelectedTile = false;
        selectedTileRow = -1;
        selectedTileCol = -1;
    }
}

void EditorViewportPanel::HandleTilePainting(
    TileMap* tileMap,
    const SDL_FRect& camera,
    const ImVec2& mousePos,
    int brushTileRow,
    int brushTileCol,
    bool& hasSelectedTile,
    int& selectedTileRow,
    int& selectedTileCol
)
{
    if (!tileMap)
    {
        return;
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        lastPaintedTileRow = -1;
        lastPaintedTileCol = -1;
        return;
    }

    float localX = mousePos.x - viewportPos.x;
    float localY = mousePos.y - viewportPos.y;

    float worldX = camera.x + localX;
    float worldY = camera.y + localY;

    int row = 0;
    int col = 0;

    if (!tileMap->WorldToGrid(worldX, worldY, row, col))
    {
        return;
    }

    if (row == lastPaintedTileRow && col == lastPaintedTileCol)
    {
        return;
    }

    TileCell& tile = tileMap->GetTile(row, col);
    tile.tilesetRow = brushTileRow;
    tile.tilesetColumn = brushTileCol;

    hasSelectedTile = true;
    selectedTileRow = row;
    selectedTileCol = col;

    lastPaintedTileRow = row;
    lastPaintedTileCol = col;
}

void EditorViewportPanel::HandleTerrainPainting(
    TileMap* tileMap,
    const SDL_FRect& camera,
    const ImVec2& mousePos,
    TerrainType terrainBrush,
    bool& hasSelectedTile,
    int& selectedTileRow,
    int& selectedTileCol
)
{
    if (!tileMap)
    {
        return;
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        lastPaintedTileRow = -1;
        lastPaintedTileCol = -1;
        return;
    }

    float localX = mousePos.x - viewportPos.x;
    float localY = mousePos.y - viewportPos.y;

    float worldX = camera.x + localX;
    float worldY = camera.y + localY;

    int row = 0;
    int col = 0;

    if (!tileMap->WorldToGrid(worldX, worldY, row, col))
    {
        return;
    }

    if (row == lastPaintedTileRow && col == lastPaintedTileCol)
    {
        return;
    }

    TileCell& tile = tileMap->GetTile(row, col);
    tile.terrain = terrainBrush;

    hasSelectedTile = true;
    selectedTileRow = row;
    selectedTileCol = col;

    lastPaintedTileRow = row;
    lastPaintedTileCol = col;
}

int EditorViewportPanel::PickEntityAtWorldPosition(
    const std::unique_ptr<Registry>& registry,
    float worldX,
    float worldY
)
{
    int bestEntityId = -1;
    int bestZIndex = -1000000;

    for (auto entity : registry->GetAllEntities())
    {
        if (entity.BelongsToGroup("tiles"))
        {
            continue;
        }

        if (!entity.HasComponent<TransformComponent>())
        {
            continue;
        }

        const auto& transform = entity.GetComponent<TransformComponent>();

        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
        int zIndex = 0;
        bool canPick = false;

        if (entity.HasComponent<SpriteComponent>())
        {
            const auto& sprite = entity.GetComponent<SpriteComponent>();

            left = transform.position.x;
            top = transform.position.y;
            right = left + sprite.width * transform.scale.x;
            bottom = top + sprite.height * transform.scale.y;
            zIndex = sprite.zIndex;

            canPick = true;
        }
        else if (entity.HasComponent<BoxColliderComponent>())
        {
            const auto& collider = entity.GetComponent<BoxColliderComponent>();

            left = transform.position.x + collider.offset.x;
            top = transform.position.y + collider.offset.y;
            right = left + collider.width * transform.scale.x;
            bottom = top + collider.height * transform.scale.y;

            if (entity.HasComponent<SpriteComponent>())
            {
                zIndex = entity.GetComponent<SpriteComponent>().zIndex;
            }

            canPick = true;
        }

        if (!canPick)
        {
            continue;
        }

        bool hit =
            worldX >= left &&
            worldX <= right &&
            worldY >= top &&
            worldY <= bottom;

        if (hit && zIndex >= bestZIndex)
        {
            bestZIndex = zIndex;
            bestEntityId = entity.GetId();
        }
    }

    return bestEntityId;
}

void EditorViewportPanel::HandleViewportSelectAndDragging(
    const std::unique_ptr<Registry>& registry,
    ProjectModule* projectModule,
    const SDL_FRect& camera,
    const ImVec2& mousePos,
    EditorSessionState& state
)
{
    float localX = mousePos.x - viewportPos.x;
    float localY = mousePos.y - viewportPos.y;

    float worldX = camera.x + localX;
    float worldY = camera.y + localY;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        int pickedEntityId = PickEntityAtWorldPosition(registry, worldX, worldY);
        state.selectedEntityId = pickedEntityId;
        state.selectedProjectObjectId = -1;
        isDraggingEntity = false;

        if (state.selectedEntityId >= 0)
        {
            if (projectModule)
            {
                projectModule->SetSelectedProjectObject(-1);
            }

            Entity selectedEntity = FindEntityById(registry, state.selectedEntityId);

            if (selectedEntity.GetId() >= 0 &&
                selectedEntity.HasComponent<TransformComponent>())
            {
                auto& transform = selectedEntity.GetComponent<TransformComponent>();

                dragOffsetX = worldX - transform.position.x;
                dragOffsetY = worldY - transform.position.y;
                isDraggingEntity = true;
            }
        }
        else if (projectModule)
        {
            const int pickedProjectObjectId = projectModule->PickProjectObject(glm::vec2(worldX, worldY), 18.0f);
            state.selectedProjectObjectId = pickedProjectObjectId;
            projectModule->SetSelectedProjectObject(pickedProjectObjectId);
        }
    }

    if (isDraggingEntity && ImGui::IsMouseDown(ImGuiMouseButton_Left) && state.selectedEntityId >= 0)
    {
        Entity selectedEntity = FindEntityById(registry, state.selectedEntityId);

        if (selectedEntity.GetId() >= 0 &&
            selectedEntity.HasComponent<TransformComponent>())
        {
            auto& transform = selectedEntity.GetComponent<TransformComponent>();

            transform.position.x = worldX - dragOffsetX;
            transform.position.y = worldY - dragOffsetY;
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        isDraggingEntity = false;
    }
}

void EditorViewportPanel::HandleProjectClassDrop(
    const std::unique_ptr<Registry>& registry,
    const ClassRegistry& classRegistry,
    const SDL_FRect& camera,
    const ImVec2& mousePos,
    EditorSessionState& state
)
{
    if (!ImGui::BeginDragDropTarget())
    {
        return;
    }

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PIPEFRAME_PROJECT_CLASS");
    if (payload && payload->Data && payload->DataSize > 0)
    {
        const char* typeName = static_cast<const char*>(payload->Data);
        const float localX = mousePos.x - viewportPos.x;
        const float localY = mousePos.y - viewportPos.y;
        const glm::vec2 worldPosition(camera.x + localX, camera.y + localY);

        Entity entity = classRegistry.CreateEntity(typeName, *registry, worldPosition);
        state.selectedEntityId = entity.GetId();
        state.hasSelectedTile = false;
        state.activeTool = EditorTool::EntitySelect;
    }

    ImGui::EndDragDropTarget();
}

void EditorViewportPanel::Draw(
    SDL_Texture* viewportTexture,
    const std::unique_ptr<Registry>& registry,
    TileMap* tileMap,
    const std::unordered_map<std::string, FieldGrid>& fieldGrids,
    const ClassRegistry& classRegistry,
    ProjectModule* projectModule,
    SDL_FRect& camera,
    EditorSessionState& state
)
{
    ImGui::Begin("Viewport");

    viewportPos = ImGui::GetCursorScreenPos();
    viewportSize = ImGui::GetContentRegionAvail();
    viewportHovered = ImGui::IsWindowHovered();

    if (viewportTexture && viewportSize.x > 0 && viewportSize.y > 0)
    {
        ImGui::Image(reinterpret_cast<ImTextureID>(viewportTexture), viewportSize);

        ImVec2 mousePos = ImGui::GetMousePos();
        HandleProjectClassDrop(registry, classRegistry, camera, mousePos, state);

        viewportHovered =
            mousePos.x >= viewportPos.x &&
            mousePos.x <= viewportPos.x + viewportSize.x &&
            mousePos.y >= viewportPos.y &&
            mousePos.y <= viewportPos.y + viewportSize.y;

        if (viewportHovered)
        {
            UpdateHoveredTile(tileMap, camera, mousePos);
            HandleViewportPanning(camera, tileMap, mousePos);

            if (!isPanningViewport)
            {
                if (state.activeTool == EditorTool::EntitySelect)
                {
                    HandleViewportSelectAndDragging(registry, projectModule, camera, mousePos, state);
                }
                else if (state.activeTool == EditorTool::TileSelect)
                {
                    HandleTileSelection(tileMap, camera, mousePos, state.hasSelectedTile, state.selectedTileRow,
                                        state.selectedTileCol,
                                        state.selectedEntityId);
                }
                else if (state.activeTool == EditorTool::TilePaint)
                {
                    HandleTilePainting(tileMap, camera, mousePos, state.brushTileRow, state.brushTileCol,
                                       state.hasSelectedTile,
                                       state.selectedTileRow, state.selectedTileCol);
                }
                else if (state.activeTool == EditorTool::TerrainPaint)
                {
                    HandleTerrainPainting(tileMap, camera, mousePos, state.terrainBrush, state.hasSelectedTile,
                                          state.selectedTileRow,
                                          state.selectedTileCol);
                }
            }
        }
        else
        {
            hoveredTileRow = -1;
            hoveredTileCol = -1;
        }

        DrawTerrainOverlay(tileMap, camera, state.showTerrainOverlay);
        DrawFieldOverlay(fieldGrids, camera, state.showFieldOverlay);
        DrawColliderOverlay(registry, camera, state.showColliderOverlay);
        DrawWorldBounds(tileMap, camera);
        DrawHoveredTileOutline(tileMap, camera);
        DrawSelectedEntityOutline(registry, camera, state.selectedEntityId);
        DrawSelectedTileOutline(tileMap, camera, state.hasSelectedTile, state.selectedTileRow, state.selectedTileCol);
    }
    else
    {
        ImGui::Text("Viewport texture not ready");
    }

    ImGui::End();
}
