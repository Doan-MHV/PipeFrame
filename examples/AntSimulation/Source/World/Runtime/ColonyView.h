#pragma once
#include <PipeFrame/ECS/ComponentView.h>
#include <PipeFrame/Components/Transform2DComponent.h>
#include "Components/ColonyStateComponent.h"
#include "Components/ColonyHistoryComponent.h"
#include "Configuration/AntConfiguration.h"
namespace ant_simulation {
// Borrowed view: all data is owned by the engine scene.
class ColonyView : public pipeframe::ComponentView<ColonyStateComponent, ColonyHistoryComponent, pipeframe::Transform2DComponent> {
public:
    explicit ColonyView(pipeframe::SceneObject object) : ComponentView(object) {}
    void Initialize(ColonyId, pipeframe::Vector2f, pipeframe::Color,
                    const AntConfiguration &, std::size_t collectionWindowSamples = 60);
    ColonyStateComponent &State() const { return Require<ColonyStateComponent>(); }
    ColonyHistoryComponent &History() const { return Require<ColonyHistoryComponent>(); }
    pipeframe::Transform2DComponent &Transform() const { return Require<pipeframe::Transform2DComponent>(); }
    void SetMemberCount(std::size_t count) { State().memberCount = count; }
    std::size_t GetMemberCount() const { return State().memberCount; }
    void SetPosition(pipeframe::Vector2f value) { Transform().position = value; }
    void AddFood(float quantity);

    void UpdateCollectionRate();

    [[nodiscard]]
    std::size_t AcquireNameSuffix(
        std::size_t nameIndex
    );

    [[nodiscard]]
    ColonyId GetId() const;

    [[nodiscard]]
    pipeframe::Vector2f GetPosition() const;

    [[nodiscard]] ColonyId GetGroupId() const { return State().id; }

    [[nodiscard]]
    pipeframe::Color GetColor() const;

    [[nodiscard]]
    float GetRadius() const;

    [[nodiscard]]
    float GetReserve() const;

    [[nodiscard]]
    float GetFoodQuantity() const;

    [[nodiscard]]
    float GetCollectionRate() const;

    [[nodiscard]]
    std::size_t GetAntCount() const;

    void SetRadius(float radius);

    void SetReserve(float reserve);

    void SetAntCount(std::size_t antCount);

private:

};
} // namespace ant_simulation
