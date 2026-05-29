#include "TextureImportDialog.h"

#include <algorithm>

#include "imgui.h"

void TextureImportDialog::Open()
{
    ImGui::OpenPopup("Import Texture");
}

TextureImportResult TextureImportDialog::Draw()
{
    TextureImportResult result;

    if (!ImGui::BeginPopupModal("Import Texture", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return result;
    }

    ImGui::TextUnformatted("Add a texture to the active project asset manifest.");
    ImGui::Spacing();

    ImGui::InputText("Asset ID", assetIdBuffer, sizeof(assetIdBuffer));
    ImGui::InputText("Source File", sourceFilePathBuffer, sizeof(sourceFilePathBuffer));

    const char* modes[] = {"Single Image", "Sprite Sheet"};
    ImGui::Combo("Texture Type", &modeIndex, modes, IM_ARRAYSIZE(modes));

    ImGui::InputInt("Display Width", &displayWidth);
    ImGui::InputInt("Display Height", &displayHeight);
    displayWidth = std::max(1, displayWidth);
    displayHeight = std::max(1, displayHeight);

    if (modeIndex == 1)
    {
        ImGui::SeparatorText("Sprite Sheet");
        ImGui::InputInt("Frame Width", &frameWidth);
        ImGui::InputInt("Frame Height", &frameHeight);
        ImGui::InputInt("Default Frame", &defaultFrame);

        frameWidth = std::max(1, frameWidth);
        frameHeight = std::max(1, frameHeight);
        defaultFrame = std::max(0, defaultFrame);
    }

    ImGui::Spacing();

    if (ImGui::Button("Import Texture"))
    {
        result.requestedImport = true;
        result.assetId = assetIdBuffer;
        result.sourceFilePath = sourceFilePathBuffer;
        result.mode = modeIndex == 1 ? TextureImportMode::SpriteSheet : TextureImportMode::SingleImage;
        result.displayWidth = displayWidth;
        result.displayHeight = displayHeight;
        result.frameWidth = frameWidth;
        result.frameHeight = frameHeight;
        result.defaultFrame = defaultFrame;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    return result;
}
