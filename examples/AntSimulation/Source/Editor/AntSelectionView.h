#ifndef ANT_SELECTION_VIEW_H
#define ANT_SELECTION_VIEW_H
#include "Editor/AntInspector.h"
#include <PipeFrame/UI/View.h>
namespace ant_simulation {
inline pipeframe::ui::View AntSelectionView(AntInspector &inspector) {
    using namespace pipeframe::ui::views;
    const auto &state = inspector.GetData();
    return pipeframe::ui::views::Wrap(
        "ant.selection-actions",
        {Toggle("follow", "FOLLOW", state.follow, [&inspector](bool value) { inspector.SetFollowEnabled(value); })
             .Enabled(state.available),
         Toggle("highlight", "HIGHLIGHT", state.highlight,
                [&inspector](bool value) { inspector.SetHighlightEnabled(value); })
             .Enabled(state.available),
         Toggle("target", "TARGET", state.showTarget, [&inspector](bool value) {
             inspector.SetShowTargetEnabled(value);
         }).Enabled(state.available)});
}
} // namespace ant_simulation
#endif
