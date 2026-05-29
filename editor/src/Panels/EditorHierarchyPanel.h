

#ifndef PIPEFRAME_EDITORHIERARCHYPANEL_H
#define PIPEFRAME_EDITORHIERARCHYPANEL_H


#include <memory>
#include <string>

#include <SDL3/SDL_rect.h>

#include "EditorSessionState.h"
#include "Reflection/EditorMetadata.h"

class Registry;
class PrefabRegistry;

struct EditorHierarchyResult
{
    bool requestedPrefabSave = false;
    int prefabSourceEntityId = -1;
    std::string prefabName;
};

class EditorHierarchyPanel
{
private:
    int selectedPrefabIndex = 0;
    int selectedProjectClassIndex = 0;
    int prefabSourceEntityId = -1;
    char prefabNameBuffer[128] = {};

public:
    EditorHierarchyResult Draw(
        const std::unique_ptr<Registry>& registry,
        EditorSessionState& state,
        const PrefabRegistry& prefabRegistry,
        const ClassRegistry& classRegistry,
        const ComponentRegistry& componentRegistry,
        const SDL_FRect& camera
    );
};


#endif //PIPEFRAME_EDITORHIERARCHYPANEL_H
