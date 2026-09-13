#ifndef ANT_INSPECTOR_H
#define ANT_INSPECTOR_H

#include <optional>
#include <span>
#include <string>

#include <PipeFrame/Foundation/MathTypes.h>

#include "Configuration/AntConfiguration.h"
#include "World/Runtime/AntView.h"

namespace ant_simulation {

struct AntLegInspectorData {
    pipeframe::Vector2f start{};
    pipeframe::Vector2f end{};
};

struct AntInspectorData {
    bool available{false};

    AntId id{InvalidAntId};
    ColonyId colonyId{InvalidColonyId};

    std::string displayName;
    std::string role;
    std::string state;

    pipeframe::Vector2f position{};
    pipeframe::Vector2f target{};

    pipeframe::Color color{pipeframe::Color::White};

    std::array<AntLegInspectorData, 6> legs{};

    float energy{0.0f};
    float energyRatio{0.0f};

    float speed{0.0f};
    float speedRatio{0.0f};

    float blockedRatio{0.0f};
    float distanceToTarget{0.0f};
    float totalTravelDistance{0.0f};

    std::size_t collectedFood{0};

    bool carryingFood{false};
    bool inEncounter{false};
    bool dead{false};

    bool follow{false};
    bool highlight{true};
    bool showTarget{false};
};

class AntInspector {
  public:
    explicit AntInspector(const AntConfiguration &configuration);

    void SetSelectedAnt(std::optional<AntId> antId);

    void SetFollowEnabled(bool enabled);

    void SetHighlightEnabled(bool enabled);

    void SetShowTargetEnabled(bool enabled);

    void Refresh(std::span<const AntView> ants);

    [[nodiscard]]
    std::optional<AntId> GetSelectedAnt() const;

    [[nodiscard]]
    const AntInspectorData &GetData() const;

    [[nodiscard]]
    const AntView *FindSelectedAnt(std::span<const AntView> ants) const;

  private:
    void ClearData();

    const AntConfiguration &configuration;

    std::optional<AntId> selectedAntId;

    bool followEnabled{false};
    bool highlightEnabled{true};
    bool showTargetEnabled{false};

    AntInspectorData data;
};

} // namespace ant_simulation

#endif