#ifndef PIPEFRAME_TEXTUREIMPORTDIALOG_H
#define PIPEFRAME_TEXTUREIMPORTDIALOG_H

#include <string>

#include "Core/EditorCommands.h"

struct TextureImportResult
{
    bool requestedImport = false;
    std::string assetId;
    std::string sourceFilePath;
    TextureImportMode mode = TextureImportMode::SingleImage;
    int displayWidth = 32;
    int displayHeight = 32;
    int frameWidth = 32;
    int frameHeight = 32;
    int defaultFrame = 0;
};

class TextureImportDialog
{
private:
    char assetIdBuffer[128] = "new-texture";
    char sourceFilePathBuffer[512] = "";
    int modeIndex = 0;
    int displayWidth = 32;
    int displayHeight = 32;
    int frameWidth = 32;
    int frameHeight = 32;
    int defaultFrame = 0;

public:
    void Open();
    TextureImportResult Draw();
};

#endif // PIPEFRAME_TEXTUREIMPORTDIALOG_H
