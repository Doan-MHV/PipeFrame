

#ifndef PIPEFRAME_COMPONENTINSPECTORS_H
#define PIPEFRAME_COMPONENTINSPECTORS_H
#include "Assets/AssetRegistry.h"
#include "ECS/Entity.h"


namespace EditorInspector
{
    void DrawTransform(Entity selectedEntity);
    void DrawRigidBody(Entity selectedEntity);
    void DrawSoftCollision(Entity selectedEntity);
    void DrawKeyboardControl(Entity selectedEntity);
    void DrawSprite(Entity selectedEntity, AssetRegistry& assetRegistry);
    void DrawHealth(Entity selectedEntity);
    void DrawAttributes(Entity selectedEntity);
    void DrawBoxCollider(Entity selectedEntity);
    void DrawMovementType(Entity selectedEntity);
    void DrawAnimation(Entity selectedEntity);
}


#endif // PIPEFRAME_COMPONENTINSPECTORS_H
