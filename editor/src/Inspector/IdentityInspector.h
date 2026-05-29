#ifndef PIPEFRAME_IDENTITYINSPECTOR_H
#define PIPEFRAME_IDENTITYINSPECTOR_H

#include <memory>
#include <string>

#include "ECS/Entity.h"
#include "Project/ProjectConfig.h"

class Registry;

class IdentityInspector
{
private:
    int inspectorEntityId = -1;
    char persistentIdBuffer[256] = {};

public:
    void SyncBuffers(Entity selectedEntity);

    void Draw(
        const std::unique_ptr<Registry>& registry,
        const ProjectConfig& projectConfig,
        Entity selectedEntity
    );

private:
    bool IsPersistentIdUnique(
        const std::unique_ptr<Registry>& registry,
        const std::string& candidate,
        int ignoreEntityId
    ) const;
};

#endif // PIPEFRAME_IDENTITYINSPECTOR_H
