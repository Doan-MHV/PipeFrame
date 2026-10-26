#pragma once
#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/UI/View.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

namespace pipeframe::ui {
// A declarative presenter. The engine host owns widgets, resources and event routing.
class ViewPanel {
public:
    ViewPanel() = default;
    ViewPanel(const ViewPanel&) = delete;
    ViewPanel& operator=(const ViewPanel&) = delete;
    virtual ~ViewPanel() = default;
    // Optional real-time UI polling; independent of paused simulation time.
    virtual void OnUpdate(float) {}
    // Requests a rebuild on the next UI update; repeated requests are coalesced.
    void RefreshView() { ++revision; }
    std::uint64_t ViewRevision() const { return revision; }
    View DescribeView() { return BuildView(); }
    pipeframe::Vector2f PreferredSize() const { return preferredSize; }
    // Host-bound actions: Hide collapses a drawer, or hides a floating panel.
    void Hide() { if (visibility) visibility(false); }
    void Show() { if (visibility) visibility(true); }
    void BindVisibility(std::function<void(bool)> callback) { visibility=std::move(callback); }
    using HitTester = std::function<std::optional<std::string>(pipeframe::Vector2f, std::string_view)>;
    void BindHitTester(HitTester value) { hitTester = std::move(value); }

protected:
    virtual View BuildView() = 0;
    void SetPreferredSize(pipeframe::Vector2f size) { preferredSize = size; }
    std::optional<std::string> HitKeyAt(pipeframe::Vector2f point, std::string_view prefix) const {
        return hitTester ? hitTester(point, prefix) : std::nullopt;
    }

private:
    std::uint64_t revision{1};
    pipeframe::Vector2f preferredSize{};
    HitTester hitTester;
    std::function<void(bool)> visibility;
};
}  // namespace pipeframe::ui
