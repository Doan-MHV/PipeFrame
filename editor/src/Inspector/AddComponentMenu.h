

#ifndef PIPEFRAME_ADDCOMPONENTMENU_H
#define PIPEFRAME_ADDCOMPONENTMENU_H


#include "Assets/AssetRegistry.h"
#include "ECS/Entity.h"
#include "Reflection/EditorMetadata.h"

enum class AddedComponentType
{
    None,
    Engine,
    Project
};

namespace EditorInspector
{
    AddedComponentType DrawAddComponentMenu(
        Entity selectedEntity,
        AssetRegistry& assetRegistry,
        const ComponentRegistry& componentRegistry
    );
}


#endif //PIPEFRAME_ADDCOMPONENTMENU_H
