#pragma once
#include <cstdint>

namespace pipeframe::ecs {
// Identity only: state lives in components and execution in systems/behaviours.
using Entity = std::uint64_t;
inline constexpr Entity InvalidEntity = 0;
}
