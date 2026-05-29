#ifndef PIPEFRAME_REMOVECOMPONENTMENU_H
#define PIPEFRAME_REMOVECOMPONENTMENU_H

#include "ECS/Entity.h"
#include "Reflection/EditorMetadata.h"

enum class RemovedComponentType
{
    None,
    Engine,
    Project
};

namespace EditorInspector
{
    RemovedComponentType DrawRemoveComponentMenu(
        Entity selectedEntity,
        const ComponentRegistry& componentRegistry
    );
}

#endif // PIPEFRAME_REMOVECOMPONENTMENU_H
