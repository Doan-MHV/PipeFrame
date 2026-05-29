#ifndef PIPEFRAME_EDITORSAVESECTION_H
#define PIPEFRAME_EDITORSAVESECTION_H

#include <memory>
#include <string>

#include "EditorSessionState.h"
#include "Game/LevelFilePaths.h"
#include "Map/TileMap.h"

class Registry;
class ComponentRegistry;

class EditorSaveSection
{
public:
    void Draw(
        EditorSessionState& state,
        const std::unique_ptr<Registry>& registry,
        const ComponentRegistry& componentRegistry,
        LevelFilePaths& levelFilePaths,
        TileMap* tileMap
    );

private:
    std::string BuildSaveAsPath(const std::string& originalPath) const;
    std::string BuildEntitiesPath(const std::string& mapPath) const;
    std::string GetEntitiesSavePath(const LevelFilePaths& levelFilePaths) const;
};

#endif // PIPEFRAME_EDITORSAVESECTION_H
