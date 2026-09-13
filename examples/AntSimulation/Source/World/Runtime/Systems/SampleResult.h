#ifndef ANT_SAMPLE_RESULT_H
#define ANT_SAMPLE_RESULT_H

#include <PipeFrame/Foundation/MathTypes.h>

namespace ant_simulation {

struct AntWorldCell;

struct SampleResult {
    const AntWorldCell *cell{nullptr};

    float angle{0.0f};

    pipeframe::Vector2f direction{
        0.0f,
        0.0f,
    };

    float distance{0.0f};
    float intensity{0.0f};

    pipeframe::Vector2f position{
        0.0f,
        0.0f,
    };

    bool earlyStop{false};

    [[nodiscard]]
    bool IsValid() const {
        return cell != nullptr;
    }
};

} // namespace ant_simulation

#endif