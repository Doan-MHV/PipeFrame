#ifndef PIPEFRAME_COLLISIONLAYERCOMPONENT_H
#define PIPEFRAME_COLLISIONLAYERCOMPONENT_H

#include "Reflection/ComponentAnnotations.h"

enum class CollisionLayer
{
    Default,
    Player,
    Enemy,
    Projectile,
    Hazard,
    Trigger
};

PF_COMPONENT(PF::Engine, PF::JsonName("collision_layer"))
struct CollisionLayerComponent
{
    PF_ENUM_PROPERTY(PF::Edit, PF::Save, "default", "player", "enemy", "projectile", "hazard", "trigger")
    CollisionLayer layer = CollisionLayer::Default;

    CollisionLayerComponent(CollisionLayer layer = CollisionLayer::Default)
    {
        this->layer = layer;
    }
};

#endif // PIPEFRAME_COLLISIONLAYERCOMPONENT_H
