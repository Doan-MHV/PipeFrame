#ifndef ANT_COLONY_INSPECTOR_H
#define ANT_COLONY_INSPECTOR_H

#include <optional>
#include <span>

#include <PipeFrame/Foundation/MathTypes.h>

#include "World/Runtime/ColonyView.h"

namespace ant_simulation {

struct ColonyInspectorData {
    bool available{false};

    ColonyId id{InvalidColonyId};

    pipeframe::Vector2f position{};

    pipeframe::Color color{pipeframe::Color::White};

    float radius{0.0f};
    float reserve{0.0f};

    float foodQuantity{0.0f};
    float collectionRate{0.0f};

    std::size_t antCount{0};

    float soldierRequested{0.0f};
};

class ColonyInspector {
  public:
    void SetSelectedColony(std::optional<ColonyId> colonyId);

    void Refresh(std::span<const ColonyView> colonies);

    bool SetPosition(std::span<ColonyView> colonies, pipeframe::Vector2f position);

    bool SetRadius(std::span<ColonyView> colonies, float radius);

    bool SetReserve(std::span<ColonyView> colonies, float reserve);

    bool SetSoldierRequested(std::span<ColonyView> colonies, float soldierRequested);

    bool SetColor(std::span<ColonyView> colonies, pipeframe::Color color);

    [[nodiscard]]
    std::optional<ColonyId> GetSelectedColony() const;

    [[nodiscard]]
    const ColonyInspectorData &GetData() const;

    [[nodiscard]]
    ColonyView *FindSelectedColony(std::span<ColonyView> colonies) const;

    [[nodiscard]]
    const ColonyView *FindSelectedColony(std::span<const ColonyView> colonies) const;

  private:
    void ClearData();

    std::optional<ColonyId> selectedColonyId;

    ColonyInspectorData data;
};

} // namespace ant_simulation

#endif