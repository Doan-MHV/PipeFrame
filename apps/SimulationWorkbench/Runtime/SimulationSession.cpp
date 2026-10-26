#include "SimulationSession.h"

namespace pipeframe::editor {

SimulationSession::SimulationSession(
    ProjectRuntimeHost &runtimeHost
)
    : runtimeHost(runtimeHost) {

    controller.Pause();
    controller.ResetTickCount();
}

void SimulationSession::Toggle(
    std::span<const SceneObjectData> objects
) {
    if (controller.IsPlaying()) {
        controller.Pause();
        runtimeHost.PausePreview();
        return;
    }

    EnsurePreview(objects);
    runtimeHost.ResumePreview();
    controller.Play();
}

void SimulationSession::RequestSingleStep(
    std::span<const SceneObjectData> objects
) {
    if (controller.IsPlaying()) {
        return;
    }

    EnsurePreview(objects);
    runtimeHost.ResumePreview();
    controller.RequestSingleStep();
}

void SimulationSession::Reset(
    std::span<const SceneObjectData> objects
) {
    controller.Pause();
    controller.ResetTickCount();

    runtimeHost.EndPreview();
    runtimeHost.ResetPreview();
    runtimeHost.SynchronizeScene(objects);
}

void SimulationSession::SetSpeed(const SimulationSpeed speed) {
    controller.SetSpeed(speed);
}

void SimulationSession::FixedUpdate(
    const float fixedDeltaTime
) {
    if (!controller.ConsumeTick()) {
        return;
    }

    runtimeHost.FixedUpdate(fixedDeltaTime);

    if (controller.IsPaused()) {
        runtimeHost.PausePreview();
    }
}

void SimulationSession::Stop() {
    controller.Pause();
    runtimeHost.EndPreview();
}

bool SimulationSession::IsPlaying() const {
    return controller.IsPlaying();
}

bool SimulationSession::IsPreviewActive() const {
    return runtimeHost.IsPreviewActive();
}

bool SimulationSession::IsFullSpeed() const {
    return controller.IsFullSpeed();
}

SimulationSpeed SimulationSession::GetSpeed() const {
    return controller.GetSpeed();
}

bool SimulationSession::CanAuthorScene() const {
    // Authored edits are synchronized by ProjectSession even during a preview.
    // Runtime implementations decide how to rebuild their domain state.
    return true;
}

const SimulationController &
SimulationSession::GetController() const {
    return controller;
}

void SimulationSession::EnsurePreview(
    std::span<const SceneObjectData> objects
) {
    if (runtimeHost.IsPreviewActive()) {
        return;
    }

    runtimeHost.SynchronizeScene(objects);
    runtimeHost.BeginPreview();

    controller.ResetTickCount();
}

} // namespace pipeframe::editor
