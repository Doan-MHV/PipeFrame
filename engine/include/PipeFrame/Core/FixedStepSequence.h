#pragma once
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace pipeframe {
// Validates a externally scheduled fixed tick. Owns ordering, not elapsed-time
// accumulation or system registration. A failed callback cannot be rolled back:
// the sequence becomes faulted and its owner must reset/recreate simulation state.
class FixedStepSequence {
public:
    explicit FixedStepSequence(std::size_t phaseCount) : count(phaseCount) {
        if (!count) throw std::invalid_argument("A fixed step requires phases");
    }
    template <class Function>
    void Execute(std::size_t phase, float delta, Function run) {
        if (faulted) throw std::logic_error("Fixed step failed; reset simulation before continuing");
        if (running || phase != next || phase >= count)
            throw std::logic_error("Fixed step phase repeated, skipped or reentered");
        if (!std::isfinite(delta) || delta <= 0 || (next && delta != step))
            throw std::invalid_argument("Fixed step phases require one finite positive timestep");
        step = delta;
        running = true;
        try {
            run();
        } catch (...) {
            running = false;
            faulted = true;
            throw;
        }
        running = false;
        if (++next == count) {
            next = 0;
            ++ticks;
        }
    }
    std::size_t CompletedTicks() const { return ticks; }
    bool IsFaulted() const { return faulted; }

private:
    std::size_t count, next{}, ticks{};
    float step{};
    bool running{}, faulted{};
};
}  // namespace pipeframe
