#pragma once
#include <PipeFrame/UI/View.h>

#include <cstdint>

namespace pipeframe::ui {
// Mounted sources are polled by the UI frame, never by simulation ticks.
// A source belongs to one mounted location. Use separate instances for independent state.
class ViewSource {
public:
    virtual ~ViewSource() = default;
    virtual const View& Build() = 0;
    virtual std::uint64_t Revision() const = 0;
    virtual void Mount() = 0;
    virtual void Unmount() noexcept = 0;
};
}  // namespace pipeframe::ui
