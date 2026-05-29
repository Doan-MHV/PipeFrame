

#ifndef PIPEFRAME_EDITORENTITYCOMPONENT_H
#define PIPEFRAME_EDITORENTITYCOMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT(PF::Engine, PF::Hidden, PF::NotSerializable)
struct EditorEntityComponent
{
    PF_PROPERTY(PF::Edit, PF::Save)
    bool save = true;

    EditorEntityComponent(bool save = true)
    {
        this->save = save;
    }
};

#endif //PIPEFRAME_EDITORENTITYCOMPONENT_H
