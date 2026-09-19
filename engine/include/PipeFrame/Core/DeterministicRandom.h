#ifndef PIPEFRAME_CORE_DETERMINISTIC_RANDOM_H
#define PIPEFRAME_CORE_DETERMINISTIC_RANDOM_H
#include <cstdint>
#include <limits>
namespace pipeframe {
class DeterministicRandom {
public:
    explicit DeterministicRandom(std::uint64_t seed = 0) : rootSeed(seed), state(Mix(seed)) {}
    [[nodiscard]] std::uint64_t NextU64() {
        state += 0x9e3779b97f4a7c15ULL;
        return Mix(state);
    }
    [[nodiscard]] std::uint32_t NextU32() { return static_cast<std::uint32_t>(NextU64() >> 32); }
    [[nodiscard]] float NextFloat() { return static_cast<float>(NextU32() >> 8) * (1.0f / 16777216.0f); }
    [[nodiscard]] int NextInt(int minimum, int maximumExclusive) {
        if (maximumExclusive <= minimum) return minimum;
        return minimum + static_cast<int>(NextU32() % static_cast<std::uint32_t>(maximumExclusive - minimum));
    }
    [[nodiscard]] DeterministicRandom Stream(std::uint64_t streamId) const {
        return DeterministicRandom(Mix(rootSeed ^ streamId));
    }

private:
    static std::uint64_t Mix(std::uint64_t value) {
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }
    std::uint64_t rootSeed;
    std::uint64_t state;
};
}  // namespace pipeframe
#endif
