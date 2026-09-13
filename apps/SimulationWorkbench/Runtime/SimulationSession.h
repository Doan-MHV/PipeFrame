#ifndef PIPEFRAME_SIMULATION_SESSION_H
#define PIPEFRAME_SIMULATION_SESSION_H

#include <span>

#include <PipeFrame/Project/ProjectTypes.h>
#include <PipeFrame/Simulation/SimulationController.h>

#include "ProjectRuntimeHost.h"

namespace pipeframe::editor {

class SimulationSession final {
public:
    explicit SimulationSession(
        ProjectRuntimeHost &runtimeHost);

    void Toggle(
        std::span<const SceneObjectData> objects);

    void RequestSingleStep(
        std::span<const SceneObjectData> objects);

    void Reset(
        std::span<const SceneObjectData> objects);

    void SetSpeed(SimulationSpeed speed);

    void FixedUpdate(float fixedDeltaTime);

    void Stop();

    bool IsPlaying() const;
    bool IsPreviewActive() const;
    bool IsFullSpeed() const;
    SimulationSpeed GetSpeed() const;
    bool CanAuthorScene() const;

    const SimulationController &
    GetController() const;

private:
    void EnsurePreview(
        std::span<const SceneObjectData> objects);

    ProjectRuntimeHost &runtimeHost;
    SimulationController controller;
};

} // namespace pipeframe::editor

#endif
