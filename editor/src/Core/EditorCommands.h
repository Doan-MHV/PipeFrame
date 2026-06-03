#ifndef PIPEFRAME_EDITORCOMMANDS_H
#define PIPEFRAME_EDITORCOMMANDS_H

#include <string>

enum class CppClassKind
{
    Component,
    ProjectSystem,
    EntitySystem,
    Event,
    EntityClass,
    DenseAgentSimulation,
    PhysicsScenario
};

enum class TextureImportMode
{
    SingleImage,
    SpriteSheet
};

struct EditorToolbarResult
{
    bool requestedModeToggle = false;
    bool requestedProjectCreate = false;
    bool requestedProjectOpen = false;
    bool requestedLevelCreate = false;
    bool requestedCppCompile = false;
    bool requestedCppClassCreate = false;
    bool requestedPrefabSave = false;
    bool requestedTextureImport = false;
    bool requestedPlaySpeedChange = false;

    int prefabSourceEntityId = -1;
    float playSpeed = 1.0f;

    std::string projectName;
    std::string projectParentDirectory;
    std::string projectFilePath;

    std::string prefabName;

    std::string textureAssetId;
    std::string textureSourceFilePath;
    TextureImportMode textureImportMode = TextureImportMode::SingleImage;
    int textureDisplayWidth = 32;
    int textureDisplayHeight = 32;
    int textureFrameWidth = 32;
    int textureFrameHeight = 32;
    int textureDefaultFrame = 0;

    bool copySampleAntAssets = false;

    CppClassKind cppClassKind = CppClassKind::Component;
    std::string cppClassName;

    std::string levelName;
    int levelRows = 16;
    int levelCols = 16;
    int levelTileSize = 32;
    float levelScale = 2.0f;
};

#endif // PIPEFRAME_EDITORCOMMANDS_H
