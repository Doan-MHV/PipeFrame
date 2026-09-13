#ifndef PIPEFRAME_EDITOR_WORKSPACE_TOOLS_PANEL_H
#define PIPEFRAME_EDITOR_WORKSPACE_TOOLS_PANEL_H

#include "SceneTypes.h"

#include <PipeFrame/Project/ProjectRuntime.h>
#include <PipeFrame/Project/PluginAPI.h>
#include <PipeFrame/Simulation/SimulationController.h>
#include <PipeFrame/UI/ViewPanel.h>
#include <array>

#include <span>

namespace pipeframe::editor {

class WorkspaceToolsPanel : public pipeframe::ui::ViewPanel {
public:
    WorkspaceToolsPanel();
    void Refresh(const ProjectRuntimeStatistics &statistics,
                 const SimulationController &simulation,
                 std::span<const SceneConnectionData> connections,
                 std::span<const ProjectRuntimeComponentEdit> telemetry,
                 const ExtensionRegistry &extensions,
                 const ActionRegistry &actions,
                 const SystemRegistry &systems);
    void SetBuildOutput(const std::string &text){if(pages[0]!=text){pages[0]=text;if(selectedTab==0)InvalidateView();}}
    void SetSelectedTab(std::size_t index);
    std::size_t GetSelectedTab() const;
    std::size_t GetTabCount() const;

private:
    std::array<std::string,8> pages;
    std::size_t selectedTab{};
    std::string search;
    const ActionRegistry *actionRegistry{};
    pipeframe::ui::View BuildView() override;
};
}
#endif
