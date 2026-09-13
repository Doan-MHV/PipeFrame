#include "World/Runtime/Systems/ColonyLifecycleSystem.h"
#include "Behaviours/ColonySpawnerBehaviour.h"
#include "Runtime/AntRegistration.h"
#include "Components/ColonySettingsComponent.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/Marker.h"

namespace ant_simulation {

ColonyLifecycleSystem::ColonyLifecycleSystem(
    AntEnvironment &sourceEnvironment,
    AntQuery &sourceAntQuery,
    const AntConfiguration &sourceConfiguration,
    const std::uint32_t randomSeed
)
    : environment(sourceEnvironment),
      antStore(sourceAntQuery),
      configuration(sourceConfiguration),
      scene(sourceAntQuery.GetScene()),
      colonyViews(scene),
      initialSeed(randomSeed) {

}

ColonyLifecycleSystem::~ColonyLifecycleSystem() { Clear(); }

ColonyView &ColonyLifecycleSystem::CreateColony(
    const ColonyId id,
    const pipeframe::Vector2f position,
    const pipeframe::Color color
) {
    if (id == InvalidColonyId || FindColony(id)) throw std::invalid_argument("Invalid or duplicate colony ID");
    colonyObjects.erase(id); // Discard a stale mapping after external scene destruction.
    colonyViews.Refresh();
    const auto object = AntEntityTypes().Spawn(scene,ColonyEntity(id, position, color, configuration));
    object.GetComponent<ColonySettingsComponent>()->seed=initialSeed;
    object.GetComponent<pipeframe::RandomStateComponent>()->generator.seed(initialSeed);
    try {
        object.Attach<ColonySpawnerBehaviour>(*this);
        colonyObjects.emplace(id, object);
    } catch (...) { object.Destroy(); throw; }
    ColonyView &colony = colonyViews.Track(object);
    nextColonyId = std::max(nextColonyId, id + 1);

    StampInitialHomeMarkers(
        colony);

    return colony;
}

ColonyView &ColonyLifecycleSystem::CreateColony(
    const pipeframe::Vector2f position,
    const pipeframe::Color color
) {
    return CreateColony(
        nextColonyId++,
        position,
        color);
}

pipeframe::SceneObject ColonyLifecycleSystem::GetColonyObject(ColonyId id) const {
    const auto found = colonyObjects.find(id);
    return found == colonyObjects.end() ? pipeframe::SceneObject{} : found->second;
}

ColonyView *ColonyLifecycleSystem::FindColony(
    const ColonyId id
) {
    const auto found = colonyObjects.find(id);
    return found != colonyObjects.end() && found->second.IsValid() ? colonyViews.Find(found->second.GetEntity()) : nullptr;
}

const ColonyView *ColonyLifecycleSystem::FindColony(const ColonyId id) const {
    const auto found = colonyObjects.find(id);
    return found != colonyObjects.end() && found->second.IsValid() ? colonyViews.Find(found->second.GetEntity()) : nullptr;
}

std::span<ColonyView>
ColonyLifecycleSystem::GetColonies() {
    return colonyViews.Items();
}

std::span<const ColonyView>
ColonyLifecycleSystem::GetColonies() const {
    return colonyViews.Items();
}

std::size_t
ColonyLifecycleSystem::GetColonyCount() const {
    return GetColonies().size();
}

std::size_t ColonyLifecycleSystem::RemoveColony(
    const ColonyId id
) {
    const auto found = colonyObjects.find(id);
    if (found == colonyObjects.end()) return 0;

    const auto object = found->second;
    colonyObjects.erase(found);
    const auto before = antStore.GetCount();
    if (object.IsValid()) object.Destroy();
    else OnColonyDestroyed(id);
    RecountAntsAndUpdateRadii();
    return before - antStore.GetCount();
}

void ColonyLifecycleSystem::OnColonyDestroyed(ColonyId id) {
    colonyObjects.erase(id);
    for (AntWorldCell &cell : environment.GetCells())
        for (Marker &marker : cell.markers)
            if (marker.colonyId == id) marker.Clear();
    (void)antStore.RemoveColony(id);
}

void ColonyLifecycleSystem::Update(
    const float deltaTime
) {
    (void)deltaTime;
    RecountAntsAndUpdateRadii();
}

void ColonyLifecycleSystem::UpdateColony(ColonyView &colony, float) {
    if (colony.State().reserve >= configuration.antCost) SpawnAnt(colony);
    colony.UpdateCollectionRate();
}

AntView *ColonyLifecycleSystem::SpawnAnt(
    ColonyView &colony
) {
    if (colony.State().reserve <
        configuration.antCost) {
        return nullptr;
    }

    auto &generator=colony.GetObject().GetComponent<pipeframe::RandomStateComponent>()->generator;
    colony.State().reserve -=
        configuration.antCost;

    const pipeframe::Vector2f spawnOffset{
        RandomFloat(generator,
            -0.5f,
            0.5f),
        RandomFloat(generator,
            -0.5f,
            0.5f),
    };

    const AntRole role =
        SelectWorkerRole(generator);

    const float initialAngle =
        RandomFloat(generator,
            0.0f,
            AntConfiguration::Pi *
                2.0f);

    const float markerOffset =
        RandomFloat(generator,
            0.0f,
            configuration
                .antMarkerDistance);

    AntView &ant =
        antStore.Create(
            colony.GetId(),
            role,
            colony.Transform().position +
                spawnOffset,
            initialAngle,
            markerOffset,
            configuration);

    ant.Identity().color = colony.State().color;

    ant.Identity().nameIndex =
        RandomNameIndex(generator);

    ant.Identity().nameSuffix =
        colony.AcquireNameSuffix(
            ant.Identity().nameIndex);

    return &ant;
}

void ColonyLifecycleSystem::RecountAntsAndUpdateRadii() {
    for (ColonyView &colony : GetColonies()) {
        colony.SetMemberCount(0);
    }

    for (const AntView &ant :
         antStore.GetAnts()) {
        ColonyView *colony =
            FindColony(
                ant.GetColonyId());

        if (colony != nullptr) {
            colony->SetMemberCount(colony->GetMemberCount() + 1);
        }
    }

    for (ColonyView &colony : GetColonies()) {
        colony.State().radius =
            static_cast<float>(GetColonyObject(colony.GetId()).GetComponent<ColonySettingsComponent>()->radius) +
            static_cast<float>(
                colony.GetMemberCount()) *
                0.002f;

        RefreshHomeMarkers(
            colony);
    }
}

void ColonyLifecycleSystem::StampInitialHomeMarkers(
    const ColonyView &colony
) {
    const int integerRadius =
        static_cast<int>(
            colony.State().radius);

    const pipeframe::Vector2i center =
        AntEnvironment::WorldToCell(
            colony.Transform().position);

    const float radiusSquared =
        colony.State().radius *
        colony.State().radius;

    for (int y = center.y - integerRadius;
         y <= center.y + integerRadius;
         ++y) {
        for (int x = center.x - integerRadius;
             x <= center.x + integerRadius;
             ++x) {
            const pipeframe::Vector2f cellCenter =
                AntEnvironment::GetCellCenter(
                    {x, y});

            const pipeframe::Vector2f difference =
                cellCenter -
                colony.Transform().position;

            const float distanceSquared =
                difference.x * difference.x +
                difference.y * difference.y;

            if (distanceSquared >=
                radiusSquared) {
                continue;
            }

            AntWorldCell *cell =
                environment.TryGetCell(x, y);

            if (cell == nullptr) {
                continue;
            }

            cell->SetPersistentMarker(
                MarkerKind::ToHome,
                std::numeric_limits<float>::max(),
                colony.GetId());
        }
    }
}

void ColonyLifecycleSystem::RefreshHomeMarkers(
    const ColonyView &colony
) {
    const int integerRadius =
        static_cast<int>(
            colony.State().radius);

    const pipeframe::Vector2i center =
        AntEnvironment::WorldToCell(
            colony.Transform().position);

    const float radiusSquared =
        colony.State().radius *
        colony.State().radius;

    for (int y = center.y - integerRadius;
         y <= center.y + integerRadius;
         ++y) {
        for (int x = center.x - integerRadius;
             x <= center.x + integerRadius;
             ++x) {
            const pipeframe::Vector2f cellCenter =
                AntEnvironment::GetCellCenter(
                    {x, y});

            const pipeframe::Vector2f difference =
                cellCenter -
                colony.Transform().position;

            const float distanceSquared =
                difference.x * difference.x +
                difference.y * difference.y;

            if (distanceSquared >=
                radiusSquared) {
                continue;
            }

            AntWorldCell *cell =
                environment.TryGetCell(x, y);

            if (cell == nullptr) {
                continue;
            }

            Marker &marker =
                cell->GetMarker(
                    MarkerKind::ToHome);

            if (marker.persistent &&
                marker.colonyId ==
                    colony.GetId()) {
                continue;
            }

            marker.colonyId =
                colony.GetId();

            marker.intensity =
                HomeMarkerIntensity;
        }
    }
}

void ColonyLifecycleSystem::Clear() {
    while (!colonyObjects.empty()) (void)RemoveColony(colonyObjects.begin()->first);
    nextColonyId = 1;
}

float ColonyLifecycleSystem::RandomFloat(std::mt19937 &generator,
    const float minimum,
    const float maximum
) {
    if (maximum <= minimum) {
        return minimum;
    }

    std::uniform_real_distribution<float>
        distribution(
            minimum,
            maximum);

    return distribution(
        generator);
}

std::size_t
ColonyLifecycleSystem::RandomNameIndex(std::mt19937 &generator) {
    std::uniform_int_distribution<std::size_t>
        distribution(
            0,
            AntNameCount - 1);

    return distribution(
        generator);
}

AntRole ColonyLifecycleSystem::SelectWorkerRole(std::mt19937 &generator) {
    std::bernoulli_distribution distribution(
        configuration.explorerProbability);

    return distribution(generator)
               ? AntRole::Explorer
               : AntRole::Follower;
}

} // namespace ant_simulation

namespace ant_simulation {
void ColonyLifecycleSystem::ApplySettings(ColonyId id,bool initialize) {
    auto *colony=FindColony(id);
    const auto object=GetColonyObject(id);
    if (!colony || !object.IsValid()) return;
    const auto &settings=*object.GetComponent<ColonySettingsComponent>();
    colony->State().radius=static_cast<float>(settings.radius)+static_cast<float>(colony->GetMemberCount())*0.002f;
    colony->State().color={static_cast<std::uint8_t>(settings.red),static_cast<std::uint8_t>(settings.green),static_cast<std::uint8_t>(settings.blue)};
    if (initialize) {
        colony->State().reserve=static_cast<float>(settings.population)*configuration.antCost;
        object.GetComponent<pipeframe::RandomStateComponent>()->generator.seed(static_cast<std::uint32_t>(settings.seed));
    }
    for (auto &ant : antStore.GetAnts()) if (ant.GetColonyId()==id) ant.Identity().color=colony->State().color;
    for (auto &cell : environment.GetCells()) {
        auto &marker=cell.GetMarker(MarkerKind::ToHome);
        if (marker.persistent && marker.colonyId==id) marker.Clear();
    }
    StampInitialHomeMarkers(*colony);
    RefreshHomeMarkers(*colony);
}
}
