#ifndef PIPEFRAME_LEARNING_EXPERIMENT_RUNTIME_H
#define PIPEFRAME_LEARNING_EXPERIMENT_RUNTIME_H

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pipeframe::learning {

class ExperimentClock {
public:
    explicit ExperimentClock(float limitSeconds = 0.0f);
    void Reset();
    void Advance(float deltaTime);
    void SetLimit(float seconds);
    [[nodiscard]] float Elapsed() const;
    [[nodiscard]] float Limit() const;
    [[nodiscard]] float Progress() const;
    [[nodiscard]] bool Expired() const;

private:
    float elapsed{};
    float limit{};
};

struct ExperimentHistorySample {
    std::uint32_t run{};
    std::uint32_t iteration{};
    float bestScore{};
    float averageScore{};
    float progress{};
    std::size_t modelNodeCount{};
    std::size_t modelConnectionCount{};
    std::map<std::string, double> metrics;

    [[nodiscard]] double Metric(std::string_view name, double fallback = 0.0) const;
};

class ExperimentHistory {
public:
    void Add(ExperimentHistorySample sample);
    void Clear();
    [[nodiscard]] std::span<const ExperimentHistorySample> Samples() const;
    bool SaveCsv(const std::filesystem::path& path, std::string& errorMessage) const;
    bool LoadCsv(const std::filesystem::path& path, std::string& errorMessage);

private:
    std::vector<ExperimentHistorySample> samples;
};

struct CheckpointMetadata {
    std::string trainerId;
    std::uint32_t version{1};
    std::uint32_t run{};
    std::uint32_t iteration{};
    std::uint64_t seed{};
    std::size_t population{};
    std::map<std::string, std::string> values;

    bool Save(const std::filesystem::path& path, std::string& errorMessage) const;
    bool Load(const std::filesystem::path& path, std::string& errorMessage);
};

template <typename Result>
class BackgroundEvaluation {
public:
    BackgroundEvaluation() = default;
    ~BackgroundEvaluation() { Wait(); }
    BackgroundEvaluation(const BackgroundEvaluation&) = delete;
    BackgroundEvaluation& operator=(const BackgroundEvaluation&) = delete;

    template <typename Work>
    bool Start(Work&& work) {
        if (Busy()) {
            return false;
        }
        future = std::async(std::launch::async, std::forward<Work>(work));
        return true;
    }

    [[nodiscard]] bool Busy() const { return future.valid(); }
    [[nodiscard]] bool Ready() const {
        return future.valid() && future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
    Result Take() { return future.get(); }
    void Wait() {
        if (future.valid()) {
            future.wait();
            future = {};
        }
    }

private:
    std::future<Result> future;
};

}  // namespace pipeframe::learning

#endif
