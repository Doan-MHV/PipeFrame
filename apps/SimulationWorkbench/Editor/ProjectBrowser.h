#ifndef PIPEFRAME_PROJECT_BROWSER_H
#define PIPEFRAME_PROJECT_BROWSER_H

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include <PipeFrame/UI/ViewPanel.h>

namespace pipeframe::editor {

class ProjectBrowser : public pipeframe::ui::ViewPanel {
public:
    using ActionCallback =
        std::function<void()>;

    using OpenRecentCallback =
        std::function<void(
            const std::filesystem::path &)>;

    ProjectBrowser();

    void SetOnNewProject(
        ActionCallback callback);

    void SetOnOpenProject(
        ActionCallback callback);

    void SetOnOpenRecent(
        OpenRecentCallback callback);

    void SetRecentProjects(
        const std::vector<
            std::filesystem::path>
            &projects);

    void SetMessage(
        const std::string &message);

protected:
    pipeframe::ui::View BuildView() override;

private:
    std::string message;
    std::vector<std::filesystem::path>
        recentProjectPaths;

    ActionCallback onNewProject;
    ActionCallback onOpenProject;
    OpenRecentCallback onOpenRecent;
};

} // namespace pipeframe::editor

#endif