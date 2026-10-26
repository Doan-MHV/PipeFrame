#ifndef SAILBOAT_AGENT_H
#define SAILBOAT_AGENT_H

#include <cstddef>
#include <string>

#include <PipeFrame/Learning/Genome.h>
#include <PipeFrame/Learning/Network.h>
#include "Components/Boat.h"
#include "Components/BoatEnvironment.h"
#include "SailBoatRaceTask.h"
#include "World/RaceCourse.h"

namespace sailboat_simulation {

class SailBoatAgent final {
public:
    SailBoatAgent() = default;
    explicit SailBoatAgent(std::size_t id);

    bool Compile(std::string *errorMessage = nullptr);
    void Reset(const RaceCourse &course);
    void Update(const BoatEnvironment &environment, const RaceCourse &course,
                float angularSpeedDegrees, float deltaTime, bool recordTrajectory);

    [[nodiscard]] std::size_t GetId() const;
    [[nodiscard]] float GetScore() const;
    [[nodiscard]] bool IsActive() const;
    [[nodiscard]] const Boat &GetBoat() const;
    [[nodiscard]] Boat &GetBoat();
    [[nodiscard]] const SailBoatRaceTask &GetTask() const;
    [[nodiscard]] SailBoatRaceTask &GetTask();
    [[nodiscard]] const pipeframe::learning::Genome &GetGenome() const;
    [[nodiscard]] pipeframe::learning::Genome &GetGenome();
    [[nodiscard]] const pipeframe::learning::Network &GetNetwork() const;

private:
    std::size_t id{0};
    pipeframe::learning::Genome genome{4, 1};
    pipeframe::learning::Network network;
    Boat boat;
    SailBoatRaceTask task;
};

} // namespace sailboat_simulation

#endif
