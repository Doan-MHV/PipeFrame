#include <PipeFrame/Simulation/SimulationController.h>

void SimulationController::Play() {
    state = SimulationState::Playing;
    singleStepRequested = false;
}

void SimulationController::Pause() {
    state = SimulationState::Paused;
}

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

bool SimulationController::IsPlaying() const {
    return state == SimulationState::Playing;
}

bool SimulationController::IsPaused() const {
    return state == SimulationState::Paused;
}

void SimulationController::SetSpeed(const SimulationSpeed newSpeed) {
    speed = newSpeed;
}

void SimulationController::CycleSpeed() {
    switch (speed) {
    case SimulationSpeed::Realtime:
        speed = SimulationSpeed::Double;
        break;
    case SimulationSpeed::Double:
        speed = SimulationSpeed::Quadruple;
        break;
    case SimulationSpeed::Quadruple:
        speed = SimulationSpeed::Maximum;
        break;
    case SimulationSpeed::Maximum:
        speed = SimulationSpeed::Realtime;
        break;
    }
}

SimulationSpeed SimulationController::GetSpeed() const {
    return speed;
}

float SimulationController::GetTimeScale() const {
    switch (speed) {
    case SimulationSpeed::Realtime:
        return 1.0f;
    case SimulationSpeed::Double:
        return 2.0f;
    case SimulationSpeed::Quadruple:
        return 4.0f;
    case SimulationSpeed::Maximum:
        return 1.0f;
    }

    return 1.0f;
}

const char *SimulationController::GetSpeedName() const {
    switch (speed) {
    case SimulationSpeed::Realtime:
        return "1X";
    case SimulationSpeed::Double:
        return "2X";
    case SimulationSpeed::Quadruple:
        return "4X";
    case SimulationSpeed::Maximum:
        return "MAX";
    }

    return "1X";
}

void SimulationController::SetFullSpeed(
    const bool enabled
) {
    speed = enabled ? SimulationSpeed::Maximum : SimulationSpeed::Realtime;
}

void SimulationController::ToggleFullSpeed() {
    SetFullSpeed(!IsFullSpeed());
}

bool SimulationController::IsFullSpeed() const {
    return speed == SimulationSpeed::Maximum;
}

std::uint64_t
SimulationController::GetTickCount() const {
    return tickCount;
}

void SimulationController::ResetTickCount() {
    tickCount = 0;
}
