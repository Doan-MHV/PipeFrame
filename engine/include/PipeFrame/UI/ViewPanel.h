#pragma once
#include <PipeFrame/UI/View.h>
#include <PipeFrame/Foundation/MathTypes.h>
#include <functional>
#include <optional>
#include <string_view>
#include <cstdint>

namespace pipeframe::ui {
// A declarative presenter. The engine host owns widgets, resources and event routing.
class ViewPanel {
public:
    ViewPanel() = default;
    ViewPanel(const ViewPanel &) = delete;
    ViewPanel &operator=(const ViewPanel &) = delete;
    virtual ~ViewPanel() = default;
    void InvalidateView() { ++revision; }
    std::uint64_t ViewRevision() const { return revision; }
    View DescribeView() { return BuildView(); }
    pipeframe::Vector2f PreferredSize() const { return preferredSize; }
    using HitTester=std::function<std::optional<std::string>(pipeframe::Vector2f,std::string_view)>;
    void BindHitTester(HitTester value) { hitTester=std::move(value); }
protected:
    virtual View BuildView() = 0;
    void SetPreferredSize(pipeframe::Vector2f size) { preferredSize=size; }
    std::optional<std::string> HitKeyAt(pipeframe::Vector2f point,std::string_view prefix) const {
        return hitTester?hitTester(point,prefix):std::nullopt;
    }
private:
    std::uint64_t revision{1};
    pipeframe::Vector2f preferredSize{};
    HitTester hitTester;
};
}
