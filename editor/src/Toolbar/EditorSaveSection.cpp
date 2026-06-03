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

std::string EditorSaveSection::BuildEntitiesPath(const std::string& tileMapPath) const
{
    std::size_t slashPos = tileMapPath.find_last_of("/\\");
    std::size_t dotPos = tileMapPath.find_last_of('.');

    std::string directory;
    std::string baseName;

    if (slashPos == std::string::npos)
    {
        directory = "";
        baseName = (dotPos == std::string::npos) ? tileMapPath : tileMapPath.substr(0, dotPos);
    }
    else
    {
        directory = tileMapPath.substr(0, slashPos + 1);

        if (dotPos == std::string::npos || dotPos < slashPos)
        {
            baseName = tileMapPath.substr(slashPos + 1);
        }
        else
        {
            baseName = tileMapPath.substr(slashPos + 1, dotPos - slashPos - 1);
        }
    }

    const std::string tileMapSuffix = ".tilemap";
    if (baseName.size() >= tileMapSuffix.size() &&
        baseName.substr(baseName.size() - tileMapSuffix.size()) == tileMapSuffix)
    {
        baseName = baseName.substr(0, baseName.size() - tileMapSuffix.size());
    }

    return directory + baseName + ".entities.json";
}

std::string EditorSaveSection::GetEntitiesSavePath(const LevelFilePaths& levelFilePaths) const
{
    if (!levelFilePaths.entitiesPath.empty())
    {
        return levelFilePaths.entitiesPath.string();
    }

    return BuildEntitiesPath(levelFilePaths.tileMapPath.string());
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
        if (!tileMap || levelFilePaths.tileMapPath.empty())
        {
            Logger::Err("Tilemap source path is empty");
        }
        else
        {
            const std::string tileMapPath = levelFilePaths.tileMapPath.string();
            const bool success = TileMapSerializer::SaveTileMap(*tileMap, tileMapPath);

            if (success)
            {
                Logger::Log("Saved tilemap to " + tileMapPath);
            }
            else
            {
                Logger::Err("Failed to save tilemap " + tileMapPath);
            }
        }
    }

    ImGui::SameLine(0.0f, 8.0f);

    if (ImGui::Button("Save Map As"))
    {
        if (tileMap)
        {
            std::string defaultPath = BuildSaveAsPath(levelFilePaths.tileMapPath.string());
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
                    bool success = TileMapSerializer::SaveTileMap(*tileMap, savePath);

                    if (success)
                    {
                        levelFilePaths.tileMapPath = savePath;
                        Logger::Log("Saved tilemap to " + savePath);
                    }
                    else
                    {
                        Logger::Err("Failed to save tilemap to " + savePath);
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
            const std::string path = levelFilePaths.tileMapPath.string();

            if (path.empty())
            {
                Logger::Err("Cannot save terrain: tilemap source path is empty");
            }
            else
            {
                bool success = TileMapSerializer::SaveTileMap(*tileMap, path);

                if (success)
                {
                    Logger::Log("Saved terrain into tilemap " + path);
                }
                else
                {
                    Logger::Err("Failed to save terrain into tilemap " + path);
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
            "Tilemap: %s\nEntities: %s",
            levelFilePaths.tileMapPath.string().c_str(),
            GetEntitiesSavePath(levelFilePaths).c_str()
        );
    }
}
