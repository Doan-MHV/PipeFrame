#ifndef BASIC_SIMULATION_RUNTIME_H
#define BASIC_SIMULATION_RUNTIME_H

#include <optional>
#include <span>
#include <vector>

#include <PipeFrame/Project/ProjectRuntime.h>

#include "DemoAgent.h"
#include "Population.h"
#include "PopulationRenderer.h"

namespace basic_simulation {

class BasicSimulationRuntime final
    : public pipeframe::ProjectRuntime {

  public:
    const char *GetName() const override;
    pipeframe::ProjectPluginDescriptor GetPluginDescriptor() const override;
    bool RegisterPlugin(pipeframe::PluginRegistrar &registrar, std::string &error) override;

    bool Load(
        const pipeframe::ProjectRuntimeContext &context,
        std::string &errorMessage) override;

    std::span<
        const pipeframe::SceneObjectTypeDescriptor>
    GetSceneObjectTypes() const override;

    pipeframe::SceneObjectData
    CreateDefaultObject(
        const pipeframe::SceneObjectTypeId
            &typeId) const override;

    void SynchronizeScene(
        std::span<
            const pipeframe::SceneObjectData>
            objects) override;

    void SetSelectedObject(
        std::optional<pipeframe::SceneObjectId>
            objectId) override;

    std::optional<pipeframe::SceneObjectId>
    HitTest(
        pipeframe::Vector2f worldPosition) const override;

    void Start() override;
    void FixedUpdate(float fixedDeltaTime) override;
    void Render(RenderContext &context) override;
    void Reset() override;
    void Stop() override;
    void Unload() override;

  private:
    static std::vector<
        pipeframe::SceneObjectTypeDescriptor>
    CreateObjectTypes();

    void RebuildRuntimeObjects(
        bool forcePopulationRebuild);

    static double ReadNumber(
        const pipeframe::SceneObjectData &object,
        const std::string &key,
        double defaultValue);

    std::vector<
        pipeframe::SceneObjectTypeDescriptor>
        objectTypes = CreateObjectTypes();

    std::vector<pipeframe::SceneObjectData>
        authoredObjects;

    std::vector<DemoAgent> agents;
    std::vector<Population> populations;

    PopulationRenderer populationRenderer;

    std::optional<pipeframe::SceneObjectId>
        selectedObjectId;

    bool playing = false;
};

} // namespace basic_simulation

#endif
