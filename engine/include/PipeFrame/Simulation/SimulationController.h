#ifndef PIPEFRAME_SIMULATIONCONTROLLER_H
#define PIPEFRAME_SIMULATIONCONTROLLER_H

#include <cstdint>

enum class SimulationState {
    Playing,
    Paused
};

enum class SimulationSpeed {
    Realtime,
    Double,
    Quadruple,
    Maximum
};

class SimulationController {
public:
    void Play();
    void Pause();
    void TogglePlayPause();

    void RequestSingleStep();

    bool ConsumeTick();

    bool IsPlaying() const;
    bool IsPaused() const;

    void SetSpeed(SimulationSpeed speed);
    void CycleSpeed();
    SimulationSpeed GetSpeed() const;
    float GetTimeScale() const;
    const char *GetSpeedName() const;

    void SetFullSpeed(bool enabled);
    void ToggleFullSpeed();
    bool IsFullSpeed() const;

    std::uint64_t GetTickCount() const;
    void ResetTickCount();

private:
    SimulationState state = SimulationState::Playing;

    bool singleStepRequested = false;
    SimulationSpeed speed = SimulationSpeed::Realtime;

    std::uint64_t tickCount = 0;
};

#endif
