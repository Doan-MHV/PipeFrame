#include <PipeFrame/Project/EditorWorkspace.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <ranges>
#include <unordered_set>

namespace pipeframe::editor_workspace {
namespace {
constexpr const char *Header = "PIPEFRAME_WORKSPACE";

Rectanglef ClampBounds(Rectanglef bounds, const Rectanglef area) {
    bounds.size.x = std::clamp(bounds.size.x, 240.0f, std::max(240.0f, area.size.x));
    bounds.size.y = std::clamp(bounds.size.y, 160.0f, std::max(160.0f, area.size.y));
    bounds.position.x = std::clamp(bounds.position.x, area.position.x,
                                   area.position.x + std::max(0.0f, area.size.x - bounds.size.x));
    bounds.position.y = std::clamp(bounds.position.y, area.position.y,
                                   area.position.y + std::max(0.0f, area.size.y - bounds.size.y));
    return bounds;
}
}

void WorkspaceManager::SetError(std::string *error, std::string message) {
    if (error) *error = std::move(message);
}

DockPanelLayout *WorkspaceManager::FindMutable(const std::string &id) {
    const auto found = std::ranges::find(layout.panels, id, &DockPanelLayout::id);
    return found == layout.panels.end() ? nullptr : &*found;
}

const DockPanelLayout *WorkspaceManager::Find(const std::string &id) const {
    const auto found = std::ranges::find(layout.panels, id, &DockPanelLayout::id);
    return found == layout.panels.end() ? nullptr : &*found;
}

bool WorkspaceManager::RegisterPanel(DockPanelLayout panel, std::string *error) {
    if (panel.id.empty() || Find(panel.id)) {
        SetError(error, "Panel ID must be non-empty and unique.");
        return false;
    }
    panel.dockSize = std::clamp(panel.dockSize, 160.0f, 1200.0f);
    layout.panels.push_back(std::move(panel));
    return true;
}

bool WorkspaceManager::Show(const std::string &id, const bool visible) {
    auto *panel = FindMutable(id);
    if (!panel || panel->visible == visible) return false;
    panel->visible = visible;
    return true;
}

bool WorkspaceManager::Dock(const std::string &id, const DockSite site, std::string tabGroup) {
    auto *panel = FindMutable(id);
    if (!panel || site == DockSite::Floating) return false;
    panel->site = site;
    panel->tabGroup = std::move(tabGroup);
    panel->displayId.clear();
    return true;
}

bool WorkspaceManager::Float(const std::string &id, Rectanglef bounds, std::string displayId) {
    auto *panel = FindMutable(id);
    if (!panel) return false;
    panel->site = DockSite::Floating;
    panel->floatingBounds = bounds;
    panel->displayId = std::move(displayId);
    return true;
}

bool WorkspaceManager::Resize(const std::string &id, const float size) {
    auto *panel = FindMutable(id);
    if (!panel) return false;
    const float clamped = std::clamp(size, 160.0f, 1200.0f);
    if (panel->dockSize == clamped) return false;
    panel->dockSize = clamped;
    return true;
}

bool WorkspaceManager::SelectTab(const std::string &id) {
    auto *selected = FindMutable(id);
    if (!selected) return false;
    for (auto &panel : layout.panels)
        if (panel.tabGroup == selected->tabGroup) panel.selected = panel.id == id;
    selected->visible = true;
    return true;
}

bool WorkspaceManager::RemovePanel(const std::string &id) {
    return std::erase_if(layout.panels, [&](const auto &panel) { return panel.id == id; }) > 0;
}

const WorkspaceLayout &WorkspaceManager::GetLayout() const { return layout; }
void WorkspaceManager::SetLayout(WorkspaceLayout value) { layout = std::move(value); }
void WorkspaceManager::Reset() { layout = {}; }

void WorkspaceManager::Reconcile(const std::vector<std::string> &available,
                                 const std::vector<DisplayArea> &displays,
                                 const Rectanglef primary) {
    const std::unordered_set<std::string> ids(available.begin(), available.end());
    std::erase_if(layout.panels, [&](const auto &panel) { return !ids.contains(panel.id); });
    for (auto &panel : layout.panels) {
        panel.dockSize = std::clamp(panel.dockSize, 160.0f, 1200.0f);
        if (panel.site != DockSite::Floating) continue;
        const auto display = std::ranges::find(displays, panel.displayId, &DisplayArea::id);
        if (display == displays.end()) {
            panel.displayId.clear();
            panel.floatingBounds = ClampBounds(panel.floatingBounds, primary);
        } else {
            panel.floatingBounds = ClampBounds(panel.floatingBounds, display->workArea);
        }
    }
}

std::vector<std::string> WorkspaceManager::Validate() const {
    std::vector<std::string> errors;
    std::unordered_set<std::string> ids;
    for (const auto &panel : layout.panels) {
        if (panel.id.empty() || !ids.insert(panel.id).second) errors.push_back("Workspace has a duplicate or empty panel ID.");
        if (panel.dockSize < 160.0f) errors.push_back("Workspace panel is smaller than its minimum size.");
        if (panel.site == DockSite::Floating &&
            (panel.floatingBounds.size.x < 240.0f || panel.floatingBounds.size.y < 160.0f))
            errors.push_back("Floating panel bounds are too small.");
    }
    return errors;
}

bool WorkspaceManager::Save(const std::filesystem::path &path, std::string *error) const {
    std::error_code filesystemError;
    std::filesystem::create_directories(path.parent_path(), filesystemError);
    std::ofstream output(path);
    if (!output) { SetError(error, "Could not write workspace layout."); return false; }
    output << Header << ' ' << WorkspaceLayout::CurrentVersion << '\n'
           << std::quoted(layout.name) << ' ' << layout.dpiScale << ' ' << layout.panels.size() << '\n';
    for (const auto &panel : layout.panels)
        output << std::quoted(panel.id) << ' ' << static_cast<int>(panel.site) << ' '
               << std::quoted(panel.tabGroup) << ' ' << panel.tabOrder << ' '
               << panel.visible << ' ' << panel.selected << ' ' << panel.dockSize << ' '
               << panel.floatingBounds.position.x << ' ' << panel.floatingBounds.position.y << ' '
               << panel.floatingBounds.size.x << ' ' << panel.floatingBounds.size.y << ' '
               << std::quoted(panel.displayId) << '\n';
    if (!output.good()) { SetError(error, "Could not finish writing workspace layout."); return false; }
    return true;
}

bool WorkspaceManager::Load(const std::filesystem::path &path, std::string *error) {
    std::ifstream input(path);
    std::string header;
    WorkspaceLayout loaded;
    std::size_t count = 0;
    if (!input || !(input >> header >> loaded.version) || header != Header ||
        loaded.version != WorkspaceLayout::CurrentVersion ||
        !(input >> std::quoted(loaded.name) >> loaded.dpiScale >> count)) {
        SetError(error, "Workspace layout is invalid or unsupported.");
        return false;
    }
    for (std::size_t index = 0; index < count; ++index) {
        DockPanelLayout panel;
        int site = -1;
        if (!(input >> std::quoted(panel.id) >> site >> std::quoted(panel.tabGroup) >> panel.tabOrder
                    >> panel.visible >> panel.selected >> panel.dockSize
                    >> panel.floatingBounds.position.x >> panel.floatingBounds.position.y
                    >> panel.floatingBounds.size.x >> panel.floatingBounds.size.y
                    >> std::quoted(panel.displayId)) ||
            site < static_cast<int>(DockSite::Left) || site > static_cast<int>(DockSite::Floating)) {
            SetError(error, "Workspace panel record is invalid.");
            return false;
        }
        panel.site = static_cast<DockSite>(site);
        loaded.panels.push_back(std::move(panel));
    }
    layout = std::move(loaded);
    if (!Validate().empty()) { SetError(error, "Workspace layout failed validation."); return false; }
    return true;
}

} // namespace pipeframe::editor_workspace
