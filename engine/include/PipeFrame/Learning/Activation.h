#ifndef PIPEFRAME_LEARNING_ACTIVATION_H
#define PIPEFRAME_LEARNING_ACTIVATION_H

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace pipeframe::learning {

enum class Activation : std::uint8_t {
    None,
    Sigmoid,
    Relu,
    Tanh,
};

[[nodiscard]] inline float Activate(const Activation activation, const float value) {
    switch (activation) {
        case Activation::Sigmoid:
            return 1.0f / (1.0f + std::exp(-4.9f * value));
        case Activation::Relu:
            return std::max(0.0f, value);
        case Activation::Tanh:
            return std::tanh(value);
        case Activation::None:
        default:
            return value;
    }
}

}  // namespace pipeframe::learning

#endif
