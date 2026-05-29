#include "EditorSaveSection.h"

#include <cstdio>

#include "imgui.h"

#include "ECS/ECS.h"
#include "Game/EntitySerializer.h"
#include "Logger/Logger.h"
#include "Map/TileMapSerializer.h"
#include "Reflection/EditorMetadata.h"

std::string EditorSaveSection::BuildSaveAsPath(const std::string& originalPath) const
{
    std::size_t dotPos = originalPath.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        return originalPath;
    }

    return originalPath.substr(0, dotPos) + originalPath.substr(dotPos);
}

std::string EditorSaveSection::BuildEntitiesPath(const std::string& mapPath) const
{
    std::size_t slashPos = mapPath.find_last_of("/\\");
    std::size_t dotPos = mapPath.find_last_of('.');

    std::string directory;
    std::string baseName;

    if (slashPos == std::string::npos)
    {
        directory = "";
        baseName = (dotPos == std::string::npos) ? mapPath : mapPath.substr(0, dotPos);
    }
    else
    {
        directory = mapPath.substr(0, slashPos + 1);

        if (dotPos == std::string::npos || dotPos < slashPos)
        {
            baseName = mapPath.substr(slashPos + 1);
        }
        else
        {
            baseName = mapPath.substr(slashPos + 1, dotPos - slashPos - 1);
        }
    }

    return directory + baseName + ".entities.json";
}

std::string EditorSaveSection::GetEntitiesSavePath(const LevelFilePaths& levelFilePaths) const
{
    if (!levelFilePaths.entitiesPath.empty())
    {
        return levelFilePaths.entitiesPath.string();
    }

    return BuildEntitiesPath(levelFilePaths.mapPath.string());
}

void EditorSaveSection::Draw(
    EditorSessionState& state,
    const std::unique_ptr<Registry>& registry,
    const ComponentRegistry& componentRegistry,
    LevelFilePaths& levelFilePaths,
    TileMap* tileMap
)
{
    ImGui::TextUnformatted("Save");
    ImGui::SameLine(0.0f, 8.0f);

    if (ImGui::Button("Save Map"))
    {
        if (!tileMap || levelFilePaths.mapPath.empty())
        {
            Logger::Err("Tile map source path is empty");
        }
        else
        {
            const std::string mapPath = levelFilePaths.mapPath.string();
            const bool success = TileMapSerializer::SaveVisualMap(*tileMap, mapPath);

            if (success)
            {
                Logger::Log("Saved visual tile map to " + mapPath);
            }
            else
            {
                Logger::Err("Failed to save visual tile map " + mapPath);
            }
        }
    }

    ImGui::SameLine(0.0f, 8.0f);

    if (ImGui::Button("Save Map As"))
    {
        if (tileMap)
        {
            std::string defaultPath = BuildSaveAsPath(levelFilePaths.mapPath.string());
            std::snprintf(state.saveAsPathBuffer, sizeof(state.saveAsPathBuffer), "%s", defaultPath.c_str());
            ImGui::OpenPopup("Save Map As");
        }
    }

    if (ImGui::BeginPopupModal("Save Map As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save visual tile map as:");
        ImGui::InputText("##save_as_path", state.saveAsPathBuffer, sizeof(state.saveAsPathBuffer));

        if (ImGui::Button("Save"))
        {
            if (tileMap)
            {
                std::string savePath = state.saveAsPathBuffer;

                if (savePath.empty())
                {
                    Logger::Err("Cannot save map: path is empty");
                }
                else
                {
                    bool success = TileMapSerializer::SaveVisualMap(*tileMap, savePath);

                    if (success)
                    {
                        levelFilePaths.mapPath = savePath;
                        Logger::Log("Saved visual tile map to " + savePath);
                    }
                    else
                    {
                        Logger::Err("Failed to save visual tile map to " + savePath);
                    }
                }
            }

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0f, 8.0f);

    if (ImGui::Button("Save Terrain"))
    {
        if (tileMap)
        {
            const std::string path = levelFilePaths.terrainPath.string();

            if (path.empty())
            {
                Logger::Err("Cannot save terrain: source path is empty");
            }
            else
            {
                bool success = TileMapSerializer::SaveTerrainMap(*tileMap, path);

                if (success)
                {
                    Logger::Log("Saved terrain map to " + path);
                }
                else
                {
                    Logger::Err("Failed to save terrain map to " + path);
                }
            }
        }
    }

    ImGui::SameLine(0.0f, 8.0f);

    if (ImGui::Button("Save Entities"))
    {
        if (!tileMap)
        {
            Logger::Err("Cannot save entities: tile map is missing");
        }
        else
        {
            const std::string entityPath = GetEntitiesSavePath(levelFilePaths);

            if (entityPath.empty())
            {
                Logger::Err("Cannot save entities: entity path is empty");
            }
            else
            {
                bool success = EntitySerializer::SaveEntities(registry, entityPath, &componentRegistry);

                if (success)
                {
                    Logger::Log("Saved entities to " + entityPath);
                }
                else
                {
                    Logger::Err("Failed to save entities to " + entityPath);
                }
            }
        }
    }

    if (tileMap && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Map: %s\nEntities: %s",
            levelFilePaths.mapPath.string().c_str(),
            GetEntitiesSavePath(levelFilePaths).c_str()
        );
    }
}
