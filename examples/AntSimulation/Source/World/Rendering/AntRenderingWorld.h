#pragma once
#include <PipeFrame/World/World.h>
#include "World/Rendering/EnvironmentRenderer.h"
#include "World/Rendering/AntRenderer.h"
#include "World/Rendering/ShadowRenderer.h"
#include "World/Rendering/AntDebugRenderer.h"
#include "Editor/AntEditorToolRenderer.h"
namespace ant_simulation {
class AntRenderingWorld final : public pipeframe::RenderingWorld {
public:
    explicit AntRenderingWorld(const AntConfiguration &configuration)
        :environment(configuration),ants(configuration),editor(configuration) {
        AddLayer(environment,0);
        AddLayer(shadows,1);AddLayer(ants,1);AddLayer(debug,1);AddLayer(editor,1);
    }
    void ShowAnts(bool enabled){ants.SetLayerEnabled(enabled);shadows.SetLayerEnabled(enabled);}
    EnvironmentRenderer environment;
    AntRenderer ants;
    ShadowRenderer shadows;
    AntDebugRenderer debug;
    AntEditorToolRenderer editor;
};
}
