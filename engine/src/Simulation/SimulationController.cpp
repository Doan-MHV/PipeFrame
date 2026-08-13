#include <PipeFrame/Simulation/SimulationController.h>

void SimulationController::Play() {
    state = SimulationState::Playing;
    singleStepRequested = false;
}

void SimulationController::Pause() { state = SimulationState::Paused; }

void SimulationController::TogglePlayPause() {
    if (IsPlaying()) {
        Pause();
    } else {
        Play();
    }
}

void SimulationController::RequestSingleStep() {
    if (IsPaused()) {
        singleStepRequested = true;
    }
}

bool SimulationController::ConsumeTick() {
    if (IsPaused() && !singleStepRequested) {
        return false;
    }

    singleStepRequested = false;
    ++tickCount;

    return true;
}

bool SimulationController::IsPlaying() const { return state == SimulationState::Playing; }

bool SimulationController::IsPaused() const { return state == SimulationState::Paused; }

std::uint64_t SimulationController::GetTickCount() const { return tickCount; }

void SimulationController::ResetTickCount() { tickCount = 0; }