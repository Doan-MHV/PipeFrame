#ifndef PIPEFRAME_CORE_FIXED_STEP_SCHEDULER_H
#define PIPEFRAME_CORE_FIXED_STEP_SCHEDULER_H
#include <algorithm>
#include <cstddef>
#include <cmath>
namespace pipeframe {
class FixedStepScheduler {
public:
    explicit FixedStepScheduler(double stepSeconds, std::size_t maximumSteps=8) : step(stepSeconds), maximum(maximumSteps) {}
    template <typename Function> std::size_t Advance(double elapsedSeconds, Function function) {
        accumulator+=std::max(0.0,elapsedSeconds); std::size_t count=0;
        while (accumulator+1e-12>=step && count<maximum) { function(step); accumulator-=step; ++count; }
        if (count==maximum && accumulator>=step) accumulator=std::fmod(accumulator,step);
        return count;
    }
    [[nodiscard]] double Alpha() const { return step>0.0 ? accumulator/step : 0.0; }
    void Reset() { accumulator=0.0; }
private:
    double step; std::size_t maximum; double accumulator{};
};
} // namespace pipeframe
#endif
