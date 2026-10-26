#ifndef PIPEFRAME_CORE_PROFILER_H
#define PIPEFRAME_CORE_PROFILER_H
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
namespace pipeframe {
struct ProfileSample {
    std::size_t count{};
    double totalMilliseconds{};
};
class Profiler {
public:
    void Record(std::string_view name, double milliseconds) {
        std::lock_guard lock(mutex);
        auto& sample = samples[std::string(name)];
        ++sample.count;
        sample.totalMilliseconds += milliseconds;
    }
    [[nodiscard]] ProfileSample Get(std::string_view name) const {
        std::lock_guard lock(mutex);
        const auto it = samples.find(std::string(name));
        return it == samples.end() ? ProfileSample{} : it->second;
    }
    void Clear() {
        std::lock_guard lock(mutex);
        samples.clear();
    }

private:
    mutable std::mutex mutex;
    std::unordered_map<std::string, ProfileSample> samples;
};
class ProfileScope {
public:
    ProfileScope(Profiler& profiler, std::string_view name) : profiler(profiler), name(name), start(Clock::now()) {}
    ~ProfileScope() { profiler.Record(name, std::chrono::duration<double, std::milli>(Clock::now() - start).count()); }

private:
    using Clock = std::chrono::steady_clock;
    Profiler& profiler;
    std::string name;
    Clock::time_point start;
};
}  // namespace pipeframe
#endif
