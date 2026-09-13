#ifndef ANT_COLONY_LIFECYCLE_SYSTEM_H
#define ANT_COLONY_LIFECYCLE_SYSTEM_H

#include <cstddef>
#include <random>
#include <span>
#include <vector>

#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/Simulation/System.h>
#include <PipeFrame/ECS/Scene.h>

#include "World/Runtime/AntView.h"
#include "World/Runtime/AntQuery.h"
#include "World/Runtime/ColonyView.h"
#include "Configuration/AntConfiguration.h"
#include "World/Runtime/Environment/AntEnvironment.h"
#include "World/Runtime/Environment/Marker.h"

namespace ant_simulation {

class ColonyLifecycleSystem final : public pipeframe::FixedUpdateSystem<void> {
  public:
    static constexpr std::size_t AntNameCount{
        865
    };

    static constexpr float HomeMarkerIntensity{
        20'000.0f
    };

    ColonyLifecycleSystem(
        AntEnvironment &environment,
        AntQuery &antStore,
        const AntConfiguration &configuration,
        std::uint32_t randomSeed = 0
    );

    ~ColonyLifecycleSystem();

    ColonyView &CreateColony(
        ColonyId id,
        pipeframe::Vector2f position,
        pipeframe::Color color
    );

    ColonyView &CreateColony(
        pipeframe::Vector2f position,
        pipeframe::Color color
    );

    [[nodiscard]]
    ColonyView *FindColony(ColonyId id);
    pipeframe::SceneObject GetColonyObject(ColonyId id) const;

    [[nodiscard]]
    const ColonyView *FindColony(
        ColonyId id
    ) const;

    [[nodiscard]]
    std::span<ColonyView> GetColonies();

    [[nodiscard]]
    std::span<const ColonyView> GetColonies() const;

    [[nodiscard]]
    std::size_t GetColonyCount() const;

    [[nodiscard]]
    std::size_t RemoveColony(ColonyId id);

    [[nodiscard]] std::string_view GetSystemId() const override { return "ant.colony-lifecycle"; }
    void Update(float deltaTime) override;

    AntView *SpawnAnt(ColonyView &colony);
    void UpdateColony(ColonyView &colony, float deltaTime);
    void OnColonyDestroyed(ColonyId id);

    void RecountAntsAndUpdateRadii();
    void ApplySettings(ColonyId id, bool initialize);

    void StampInitialHomeMarkers(
        const ColonyView &colony
    );

    void RefreshHomeMarkers(
        const ColonyView &colony
    );

    void Clear();

  private:
    [[nodiscard]]
    float RandomFloat(std::mt19937 &generator,
        float minimum,
        float maximum
    );

    [[nodiscard]]
    std::size_t RandomNameIndex(std::mt19937 &generator);

    [[nodiscard]]
    AntRole SelectWorkerRole(std::mt19937 &generator);

    AntEnvironment &environment;
    AntQuery &antStore;

    const AntConfiguration &configuration;

    pipeframe::BehaviourScene &scene;
    std::unordered_map<ColonyId, pipeframe::SceneObject> colonyObjects;
    pipeframe::SceneViewCache<ColonyView, ColonyStateComponent> colonyViews;

    std::uint32_t initialSeed;

    ColonyId nextColonyId{1};
};

} // namespace ant_simulation

#endif
