#include "EditorToolbar.h"

#include <cmath>

#include "imgui.h"

namespace
{
void SameLineWithSpacing()
{
    ImGui::SameLine(0.0f, 8.0f);
}

void DrawToolbarDivider()
{
    ImGui::TextDisabled("|");
}
}

EditorToolbarResult EditorToolbar::Draw(
    EngineMode mode,
    float playSpeed,
    EditorSessionState& state,
    const std::unique_ptr<Registry>& registry,
    const ComponentRegistry& componentRegistry,
    LevelFilePaths& levelFilePaths,
    TileMap* tileMap
)
{
    EditorToolbarResult result;

    ImGui::Begin(
        "ToolBar",
        nullptr,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
    );
    if (mode != EngineMode::Edit)
    {
        ImGui::BeginDisabled();
    }

    ImGui::TextUnformatted("Project");
    SameLineWithSpacing();

    if (ImGui::Button("New Project"))
    {
        newProjectDialog.Open();
    }

    SameLineWithSpacing();

    if (ImGui::Button("Open Project"))
    {
        openProjectDialog.Open();
    }

    SameLineWithSpacing();

    if (ImGui::Button("New Level"))
    {
        newLevelDialog.Open();
    }

    SameLineWithSpacing();

    if (ImGui::Button("Compile C++"))
    {
        result.requestedCppCompile = true;
    }

    SameLineWithSpacing();

    if (ImGui::Button("Create C++ Class"))
    {
        createCppClassDialog.Open();
    }

    if (mode != EngineMode::Edit)
    {
        ImGui::EndDisabled();
    }

    SameLineWithSpacing();
    DrawToolbarDivider();
    SameLineWithSpacing();

    ImGui::Text("Mode: %s", mode == EngineMode::Edit ? "Edit" : "Play");
    SameLineWithSpacing();

    if (mode == EngineMode::Edit)
    {
        if (ImGui::Button("Play"))
        {
            result.requestedModeToggle = true;
        }
    }
    else
    {
        if (ImGui::Button("Stop"))
        {
            result.requestedModeToggle = true;
        }
    }

    SameLineWithSpacing();
    ImGui::TextDisabled("F1");
    SameLineWithSpacing();
    ImGui::TextDisabled("|");
    SameLineWithSpacing();
    ImGui::TextUnformatted("Speed");
    SameLineWithSpacing();

    const char* speedLabels[] = {"0.5x", "1x", "1.5x", "2x", "4x"};
    const float speedValues[] = {0.5f, 1.0f, 1.5f, 2.0f, 4.0f};
    int currentSpeedIndex = 1;
    float bestDistance = std::abs(playSpeed - speedValues[currentSpeedIndex]);
    for (int index = 0; index < IM_ARRAYSIZE(speedValues); index++)
    {
        const float distance = std::abs(playSpeed - speedValues[index]);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            currentSpeedIndex = index;
        }
    }

    ImGui::SetNextItemWidth(72.0f);
    if (ImGui::Combo("##PlaySpeed", &currentSpeedIndex, speedLabels, IM_ARRAYSIZE(speedLabels)))
    {
        result.requestedPlaySpeedChange = true;
        result.playSpeed = speedValues[currentSpeedIndex];
    }

    SameLineWithSpacing();
    DrawToolbarDivider();
    SameLineWithSpacing();
    ImGui::TextUnformatted("Tool");
    SameLineWithSpacing();

    if (ImGui::Button("Entity"))
    {
        state.activeTool = EditorTool::EntitySelect;
        state.hasSelectedTile = false;
    }

    SameLineWithSpacing();

    if (ImGui::Button("Tile"))
    {
        state.activeTool = EditorTool::TileSelect;
        state.selectedEntityId = -1;
    }

    SameLineWithSpacing();

    if (ImGui::Button("Paint"))
    {
        state.activeTool = EditorTool::TilePaint;
        state.selectedEntityId = -1;
    }

    SameLineWithSpacing();

    if (ImGui::Button("Terrain"))
    {
        state.activeTool = EditorTool::TerrainPaint;
        state.selectedEntityId = -1;
    }

    ImGui::Separator();

    if (mode != EngineMode::Edit)
    {
        ImGui::BeginDisabled();
    }

    ImGui::TextUnformatted("Assets");
    SameLineWithSpacing();

    if (ImGui::Button("Import Texture"))
    {
        textureImportDialog.Open();
    }

    SameLineWithSpacing();
    DrawToolbarDivider();
    SameLineWithSpacing();

    if (mode != EngineMode::Edit)
    {
        ImGui::EndDisabled();
    }

    if (mode != EngineMode::Edit)
    {
        ImGui::BeginDisabled();
    }

    ImGui::TextUnformatted("Project");
    SameLineWithSpacing();

    ImGui::TextDisabled("Use Project > Compile C++ after editing project source");

    if (mode != EngineMode::Edit)
    {
        ImGui::EndDisabled();
    }

    SameLineWithSpacing();
    DrawToolbarDivider();
    SameLineWithSpacing();

    ImGui::TextUnformatted("View");
    SameLineWithSpacing();

    ImGui::Checkbox("Terrain##Overlay", &state.showTerrainOverlay);

    SameLineWithSpacing();

    ImGui::Checkbox("Fields##Overlay", &state.showFieldOverlay);

    SameLineWithSpacing();

    ImGui::Checkbox("Paths##Overlay", &state.showPathDebugOverlay);

    SameLineWithSpacing();

    ImGui::Checkbox("Colliders##Overlay", &state.showColliderOverlay);

    SameLineWithSpacing();
    DrawToolbarDivider();
    SameLineWithSpacing();

    saveSection.Draw(state, registry, componentRegistry, levelFilePaths, tileMap);

    const NewProjectResult newProjectResult = newProjectDialog.Draw();
    if (newProjectResult.requestedCreate)
    {
        result.requestedProjectCreate = true;
        result.projectName = newProjectResult.projectName;
        result.projectParentDirectory = newProjectResult.parentDirectory;
        result.copySampleAntAssets = newProjectResult.copySampleAntAssets;
    }

    const OpenProjectResult openProjectResult = openProjectDialog.Draw();
    if (openProjectResult.requestedOpen)
    {
        result.requestedProjectOpen = true;
        result.projectFilePath = openProjectResult.projectFilePath;
    }

    const NewLevelResult newLevelResult = newLevelDialog.Draw();
    if (newLevelResult.requestedCreate)
    {
        result.requestedLevelCreate = true;
        result.levelName = newLevelResult.levelName;
        result.levelRows = newLevelResult.rows;
        result.levelCols = newLevelResult.cols;
        result.levelTileSize = newLevelResult.tileSize;
        result.levelScale = newLevelResult.scale;
    }

    const CreateCppClassResult createCppClassResult = createCppClassDialog.Draw();
    if (createCppClassResult.requestedCreate)
    {
        result.requestedCppClassCreate = true;
        result.cppClassKind = createCppClassResult.kind;
        result.cppClassName = createCppClassResult.className;
    }

    const TextureImportResult textureImportResult = textureImportDialog.Draw();
    if (textureImportResult.requestedImport)
    {
        result.requestedTextureImport = true;
        result.textureAssetId = textureImportResult.assetId;
        result.textureSourceFilePath = textureImportResult.sourceFilePath;
        result.textureImportMode = textureImportResult.mode;
        result.textureDisplayWidth = textureImportResult.displayWidth;
        result.textureDisplayHeight = textureImportResult.displayHeight;
        result.textureFrameWidth = textureImportResult.frameWidth;
        result.textureFrameHeight = textureImportResult.frameHeight;
        result.textureDefaultFrame = textureImportResult.defaultFrame;
    }

    ImGui::End();
    return result;
}
