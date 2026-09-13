#ifndef PIPEFRAME_PROJECT_EDITOR_WORKSPACE_H
#define PIPEFRAME_PROJECT_EDITOR_WORKSPACE_H

#include <PipeFrame/Foundation/MathTypes.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace pipeframe::editor_workspace {

enum class DockSite : std::uint8_t { Left, Right, Bottom, Center, Floating };

struct DisplayArea {
    std::string id;
    Rectanglef workArea;
    float dpiScale{1.0f};
};

struct DockPanelLayout {
    std::string id;
    DockSite site{DockSite::Right};
    std::string tabGroup;
    std::uint32_t tabOrder{};
    bool visible{true};
    bool selected{false};
    float dockSize{320.0f};
    Rectanglef floatingBounds{{80.0f, 176.0f}, {640.0f, 420.0f}};
    std::string displayId;
};

struct WorkspaceLayout {
    static constexpr std::uint32_t CurrentVersion = 1;
    std::uint32_t version{CurrentVersion};
    std::string name{"Default"};
    float dpiScale{1.0f};
    std::vector<DockPanelLayout> panels;
};

class WorkspaceManager final {
public:
    bool RegisterPanel(DockPanelLayout panel, std::string *error = nullptr);
    bool Show(const std::string &id, bool visible);
    bool Dock(const std::string &id, DockSite site, std::string tabGroup = {});
    bool Float(const std::string &id, Rectanglef bounds, std::string displayId = {});
    bool Resize(const std::string &id, float size);
    bool SelectTab(const std::string &id);
    bool RemovePanel(const std::string &id);

    const DockPanelLayout *Find(const std::string &id) const;
    const WorkspaceLayout &GetLayout() const;
    void SetLayout(WorkspaceLayout layout);
    void Reset();
    void Reconcile(const std::vector<std::string> &availablePanelIds,
                   const std::vector<DisplayArea> &displays,
                   Rectanglef primaryWorkArea);
    std::vector<std::string> Validate() const;

    bool Save(const std::filesystem::path &path, std::string *error = nullptr) const;
    bool Load(const std::filesystem::path &path, std::string *error = nullptr);

private:
    static void SetError(std::string *error, std::string message);
    DockPanelLayout *FindMutable(const std::string &id);
    WorkspaceLayout layout;
};

} // namespace pipeframe::editor_workspace

#endif
