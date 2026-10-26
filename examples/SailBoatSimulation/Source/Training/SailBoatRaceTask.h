#ifndef SAILBOAT_RACE_TASK_H
#define SAILBOAT_RACE_TASK_H

#include <array>
#include <cstddef>

#include <PipeFrame/Foundation/MathTypes.h>

#include "Components/Boat.h"
#include "Components/BoatEnvironment.h"
#include "World/RaceCourse.h"

namespace sailboat_simulation {

class SailBoatRaceTask final {
public:
    using NeuralInputs = std::array<float, 4>;

    void Reset();

    [[nodiscard]] NeuralInputs BuildNeuralInputs(const Boat &boat,
                                                  const BoatEnvironment &environment,
                                                  const RaceCourse &course) const;

    void Update(Boat &boat, const BoatEnvironment &environment, const RaceCourse &course,
                float rudderCommand, float angularSpeedDegrees, float deltaTime,
                bool recordTrajectory = true);

    [[nodiscard]] std::size_t GetTargetIndex() const;
    [[nodiscard]] pipeframe::Vector2f GetTargetPoint(const Boat &boat, const RaceCourse &course) const;
    [[nodiscard]] float GetScore() const;
    [[nodiscard]] float GetWorldTime() const;
    [[nodiscard]] float GetRaceTime() const;
    [[nodiscard]] float GetRaceDistance() const;
    [[nodiscard]] float GetTimeOnCurrentTarget() const;
    [[nodiscard]] bool HasFinished() const;

private:
    [[nodiscard]] float GetTargetRadius(const RaceCourse &course) const;
    [[nodiscard]] bool IsCurrentTargetReached(const Boat &boat, const RaceCourse &course) const;

    std::size_t targetIndex{0};
    float score{0.0f};
    float worldTime{0.0f};
    float raceTime{-1.0f};
    float raceDistance{0.0f};
    float timeOnCurrentTarget{0.0f};
    bool finished{false};
};

} // namespace sailboat_simulation

#endif
