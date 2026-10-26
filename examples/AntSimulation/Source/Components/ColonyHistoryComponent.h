#pragma once
#include <PipeFrame/Project/Component.h>

#include <cstddef>
#include <deque>
#include <map>
namespace ant_simulation {
struct ColonyHistoryComponent : pipeframe::Component<ColonyHistoryComponent> {
  // Inspector exposure is declared here; unlisted members stay runtime-only.
  static auto BuildSchema() {
    using namespace pipeframe;
    using K = PropertyKind;
    return Define("ant.colony-history", "Colony History").Required();
  }

  std::size_t collectionWindowSamples{60};
  std::deque<float> collectionHistory;
  std::map<std::size_t, std::size_t> nameAttribution;
};
}  // namespace ant_simulation
