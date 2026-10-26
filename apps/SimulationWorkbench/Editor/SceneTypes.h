#ifndef PIPEFRAME_EDITOR_SCENE_TYPES_H
#define PIPEFRAME_EDITOR_SCENE_TYPES_H

#include <PipeFrame/Project/ProjectTypes.h>

namespace pipeframe::editor {

using SceneObjectId = pipeframe::SceneObjectId;
using SceneObjectTypeId = pipeframe::SceneObjectTypeId;

using PropertyKind = pipeframe::PropertyKind;
using PropertyValue = pipeframe::PropertyValue;
using PropertyMap = pipeframe::PropertyMap;

using PropertyDescriptor =
    pipeframe::PropertyDescriptor;

using SceneObjectTypeDescriptor =
    pipeframe::SceneObjectTypeDescriptor;

using SceneTransform = pipeframe::SceneTransform;
using SceneObjectData = pipeframe::SceneObjectData;
using SceneComponentData = pipeframe::SceneComponentData;
using SceneComponentTypeDescriptor = pipeframe::SceneComponentTypeDescriptor;
using SceneObjectReference = pipeframe::SceneObjectReference;
using PrefabInstanceLink = pipeframe::PrefabInstanceLink;
using AssetReference = pipeframe::AssetReference;
using Color = pipeframe::Color;
using SceneSettings = pipeframe::SceneSettings;
using SceneConnectionKind = pipeframe::SceneConnectionKind;
using SceneConnectionEndpoint = pipeframe::SceneConnectionEndpoint;
using SceneConnectionData = pipeframe::SceneConnectionData;

} // namespace pipeframe::editor

#endif
