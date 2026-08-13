

#ifndef PIPEFRAME_SIMULATIONCONTROLLER_H
#define PIPEFRAME_SIMULATIONCONTROLLER_H

#include <cstdint>

enum class SimulationState { Playing, Paused };

class SimulationController {
  public:
    void Play();
    void Pause();
    void TogglePlayPause();

    void RequestSingleStep();

    // Called before running a fixed simulation tick.
    // Returns true when the tick should execute.
    bool ConsumeTick();

    bool IsPlaying() const;
    bool IsPaused() const;

    std::uint64_t GetTickCount() const;
    void ResetTickCount();

  private:
    SimulationState state = SimulationState::Playing;
    bool singleStepRequested = false;
    std::uint64_t tickCount = 0;
};

#endif // PIPEFRAME_SIMULATIONCONTROLLER_H
