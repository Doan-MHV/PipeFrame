#include "EditorAssetPanel.h"

#include <string>
#include <vector>

#include "imgui.h"

namespace
{
const char* TextureSpriteModeLabel(TextureSpriteMode mode)
{
    switch (mode)
    {
    case TextureSpriteMode::SingleImage:
        return "Single Image";
    case TextureSpriteMode::SpriteSheet:
        return "Sprite Sheet";
    }

    return "Unknown";
}
}

void EditorAssetPanel::Draw(AssetRegistry& assetRegistry)
{
    ImGui::Begin("Assets");

    ImGui::TextUnformatted("Use Toolbar > Assets > Import Texture to add images.");
    ImGui::SeparatorText("Textures");

    const std::vector<std::string> textureIds = assetRegistry.GetTextureIds();
    if (textureIds.empty())
    {
        ImGui::TextDisabled("No textures loaded");
        ImGui::End();
        return;
    }

    ImGui::Text("Loaded Textures: %d", static_cast<int>(textureIds.size()));

    for (const std::string& textureId : textureIds)
    {
        const TextureInfo* textureInfo = assetRegistry.GetTextureInfo(textureId);
        if (!textureInfo)
        {
            continue;
        }

        if (!ImGui::TreeNode(textureId.c_str()))
        {
            continue;
        }

        ImGui::Text("File: %s", textureInfo->filePath.c_str());
        ImGui::Text("Pixels: %.0f x %.0f", textureInfo->pixelWidth, textureInfo->pixelHeight);
        ImGui::Text("Sprite Type: %s", TextureSpriteModeLabel(textureInfo->sprite.mode));
        ImGui::Text(
            "Display Size: %d x %d",
            textureInfo->sprite.defaultDisplayWidth,
            textureInfo->sprite.defaultDisplayHeight
        );

        if (textureInfo->sprite.mode == TextureSpriteMode::SpriteSheet)
        {
            ImGui::Text(
                "Frame Size: %d x %d",
                textureInfo->sprite.frameWidth,
                textureInfo->sprite.frameHeight
            );
            ImGui::Text("Default Frame: %d", textureInfo->sprite.defaultFrame);
        }

        ImGui::TreePop();
    }

    ImGui::End();
}
