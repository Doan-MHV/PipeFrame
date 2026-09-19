#ifndef ANT_WORKER_BEHAVIOR_H
#define ANT_WORKER_BEHAVIOR_H

#include <random>

#include "World/Runtime/Systems/MarkerSampler.h"

namespace ant_simulation {

class AntView;
struct ForagingComponent;
class AntEnvironment;
class ColonyView;
struct AntConfiguration;
struct AntWorldCell;

class WorkerBehavior {
  public:
    WorkerBehavior(AntEnvironment &environment, const AntConfiguration &configuration);

    void Update(AntView &ant, ForagingComponent &foraging, ColonyView &colony, float deltaTime,
                std::mt19937 &randomGenerator) const;

  private:
    void CollectFood(AntView &ant, ForagingComponent &foraging) const;

    void CheckDistanceToColony(AntView &ant, ForagingComponent &foraging, ColonyView &colony) const;

    void UpdateAntMarker(AntView &ant, ForagingComponent &foraging, AntWorldCell &cell) const;

    AntEnvironment &environment;
    const AntConfiguration &configuration;
    MarkerSampler sampler;
};

} // namespace ant_simulation

#endif