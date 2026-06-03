
#ifndef PIPEFRAME_EDITORINSPECTORPANEL_H
#define PIPEFRAME_EDITORINSPECTORPANEL_H

#include <memory>
#include <string>
#include <unordered_map>

#include "IdentityInspector.h"
#include "RuntimeDebugInspector.h"
#include "TileInspector.h"
#include "EditorSessionState.h"
#include "EditorTypes.h"
#include "Assets/AssetRegistry.h"
#include "Map/TileMap.h"
#include "Project/ProjectConfig.h"
#include "Reflection/EditorMetadata.h"
#include "Fields/FieldGrid.h"


class Entity;
class ProjectModule;
class Registry;

class EditorInspectorPanel
{
private:
    IdentityInspector identityInspector;
    TileInspector tileInspector;
    RuntimeDebugInspector runtimeDebugInspector;

public:
    void Draw(
        const std::unique_ptr<Registry>& registry,
        EngineMode mode,
        const ProjectConfig& projectConfig,
        const ComponentRegistry& componentRegistry,
        TileMap* tileMap,
        std::unique_ptr<AssetRegistry>& assetRegistry,
        const std::unordered_map<std::string, FieldGrid>& fieldGrids,
        ProjectModule* projectModule,
        EditorSessionState& state
    );

private:
    const char* TerrainTypeToString(TerrainType terrain) const;
};

#endif // PIPEFRAME_EDITORINSPECTORPANEL_H
