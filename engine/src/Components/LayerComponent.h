#ifndef PIPEFRAME_LAYERCOMPONENT_H
#define PIPEFRAME_LAYERCOMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine)
struct LayerComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, -1000, 1000, 1)
    int order = 0;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 2.0f, 0.01f)
    float parallaxX = 1.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 2.0f, 0.01f)
    float parallaxY = 1.0f;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool repeatX = false;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool repeatY = false;

    PF_PROPERTY(PF::Edit, PF::Save)
    bool drawBeforeTileMap = false;

    LayerComponent(
        int order = 0,
        float parallaxX = 1.0f,
        float parallaxY = 1.0f,
        bool repeatX = false,
        bool repeatY = false,
        bool drawBeforeTileMap = false
    )
    {
        this->order = order;
        this->parallaxX = parallaxX;
        this->parallaxY = parallaxY;
        this->repeatX = repeatX;
        this->repeatY = repeatY;
        this->drawBeforeTileMap = drawBeforeTileMap;
    }
};

#endif // PIPEFRAME_LAYERCOMPONENT_H
