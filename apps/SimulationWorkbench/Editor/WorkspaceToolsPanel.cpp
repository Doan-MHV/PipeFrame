#include "WorkspaceToolsPanel.h"
#include <sstream>
namespace pipeframe::editor {
using namespace pipeframe::ui;
WorkspaceToolsPanel::WorkspaceToolsPanel()  {
    pages[0]="No messages. Build and runtime diagnostics appear here.";
    pages[1]="Start a simulation to collect timing data.";
    pages[2]="No learning experiment is registered by this project.";
    pages[3]="The active simulation viewport uses the center workspace.";
    pages[4]="No mechanical, power, or signal connections.";
    pages[5]="No live telemetry values are available.";
    pages[7]="No project extensions or registered systems.";
}
void WorkspaceToolsPanel::Refresh(const ProjectRuntimeStatistics &statistics,
                                  const SimulationController &simulation,
                                  const std::span<const SceneConnectionData> connections,
                                  const std::span<const ProjectRuntimeComponentEdit> telemetry,
                                  const ExtensionRegistry &extensions,
                                  const ActionRegistry &actions,
                                  const SystemRegistry &systems) {
    const auto previousPage=pages[selectedTab];
    const auto previousRegistry=actionRegistry;
    std::ostringstream profiler;
    profiler << (simulation.IsPlaying() ? "PLAYING" : "STOPPED")
             << "  speed " << simulation.GetTimeScale() << "x\n"
             << "Visible " << statistics.visibleObjectCount << " / " << statistics.candidateObjectCount << '\n'
             << "Movement " << statistics.movementTimeMs << " ms\n"
             << "Spatial " << statistics.spatialGridTimeMs << " ms\n"
             << "Geometry " << statistics.geometryTimeMs << " ms";
    pages[1]=profiler.str();

    std::ostringstream connectionText;
    connectionText << connections.size() << " authored connection(s)";
    for (const auto &connection : connections)
        connectionText << '\n' << connection.id << "  " << connection.from.objectId << ':'
                       << connection.from.attachmentId << " -> " << connection.to.objectId << ':'
                       << connection.to.attachmentId;
    pages[4]=connectionText.str();

    std::ostringstream telemetryValue;
    telemetryValue << telemetry.size() << " live value(s)";
    for (const auto &value : telemetry)
        telemetryValue << '\n' << value.objectId << "  " << value.componentTypeId << '.' << value.propertyKey;
    pages[5]=telemetryValue.str();

    actionRegistry=&actions;

    std::ostringstream extensionValue;
    extensionValue<<extensions.All().size()<<" extension(s)  |  "<<systems.All().size()<<" system(s)";
    for(const auto &extension:extensions.All())extensionValue<<'\n'<<extension.displayName<<"  ["<<extension.id<<']';
    for(const auto &system:systems.All())extensionValue<<'\n'<<system.id<<"  (system)";
    pages[7]=extensionValue.str();
    if (pages[selectedTab]!=previousPage || actionRegistry!=previousRegistry || selectedTab==6) InvalidateView();
}

View WorkspaceToolsPanel::BuildView() {
    const char *names[]{"CONSOLE","PROFILE","LEARN","GAME","CONNECT","TELEM","COMMANDS","EXTEND"};
    std::vector<View> tabs,content;
    for (std::size_t i=0;i<8;++i) tabs.push_back(views::Button("tab:"+std::to_string(i),names[i],
        [this,i]{SetSelectedTab(i);}).Selected(i==selectedTab));
    if (selectedTab==6) {
        content.push_back(views::TextField("search","Search commands",search,[this](const std::string &value){search=value;InvalidateView();}));
        if (actionRegistry) for (const auto *action:actionRegistry->Search(search))
            content.push_back(views::Button(action->id,action->displayName+(action->defaultShortcut.empty()?"":" | "+action->defaultShortcut),
                [this,id=action->id]{if(actionRegistry)actionRegistry->Invoke(id);}));
    } else content.push_back(views::Text("body",pages[selectedTab]).FitHeight());
    return views::Column("tools",{views::Wrap("tabs",std::move(tabs),96),
        views::Scroll("page:"+std::to_string(selectedTab),views::Column("content",std::move(content)).Padding(12)).Expanded()
    }).FillHeight();
}
void WorkspaceToolsPanel::SetSelectedTab(std::size_t index) { selectedTab=std::min(index,std::size_t{7}); InvalidateView(); }
std::size_t WorkspaceToolsPanel::GetSelectedTab() const { return selectedTab; }
std::size_t WorkspaceToolsPanel::GetTabCount() const { return 8; }
}
