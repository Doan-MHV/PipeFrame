#pragma once
#include <PipeFrame/UI/View.h>

#include <cstdint>

namespace pipeframe::ui {
namespace detail {
struct ViewMountState {
    View view;
    float x{}, y{}, width{}, height{};
    std::uint64_t revision{1};
    bool active{true};
};
}  // namespace detail
// Non-owning control handle. The UI host owns the mount and disposes it on teardown.
// SetView and Unmount take effect at the next UI update/input boundary.
class MountedView {
public:
    MountedView() = default;
    explicit MountedView(const std::shared_ptr<detail::ViewMountState>& state) : state(state) {}
    bool IsMounted() const {
        const auto value = state.lock();
        return value && value->active;
    }
    void SetView(View view) const {
        ValidateView(view);
        auto value = Require();
        value->view = std::move(view);
        ++value->revision;
    }
    void SetBounds(float x, float y, float width, float height) const {
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height) || width < 0 ||
            height < 0)
            throw std::invalid_argument("Mount bounds must be finite with nonnegative size");
        auto value = Require();
        value->x = x;
        value->y = y;
        value->width = width;
        value->height = height;
    }
    void Unmount() const {
        if (auto value = state.lock()) value->active = false;
    }

private:
    std::shared_ptr<detail::ViewMountState> Require() const {
        auto value = state.lock();
        if (!value || !value->active) throw std::logic_error("View is unmounted");
        return value;
    }
    std::weak_ptr<detail::ViewMountState> state;
};
}  // namespace pipeframe::ui
