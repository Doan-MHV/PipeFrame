#ifndef PIPEFRAME_LEARNING_TRAINING_INTERFACES_H
#define PIPEFRAME_LEARNING_TRAINING_INTERFACES_H

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <PipeFrame/Learning/Network.h>

namespace pipeframe::learning {

class Policy {
  public:
    virtual ~Policy() = default;
    virtual bool Evaluate(std::span<const float> observation, std::vector<float> &action) = 0;
    [[nodiscard]] virtual InferenceSnapshot Inspect() const { return {}; }
};

class NetworkPolicy final : public Policy {
  public:
    explicit NetworkPolicy(Network network) : network(std::move(network)) {}
    bool Evaluate(const std::span<const float> observation, std::vector<float> &action) override {
        if (!network.Execute(observation)) return false;
        const auto outputs = network.GetOutputs();
        action.assign(outputs.begin(), outputs.end());
        return true;
    }
    [[nodiscard]] InferenceSnapshot Inspect() const override { return network.GetInferenceSnapshot(); }
    [[nodiscard]] Network &GetNetwork() { return network; }
    [[nodiscard]] const Network &GetNetwork() const { return network; }

  private:
    Network network;
};

class Environment {
  public:
    virtual ~Environment() = default;
    virtual void Reset(std::uint64_t seed) = 0;
    [[nodiscard]] virtual std::span<const float> Observe() const = 0;
    virtual void Step(std::span<const float> action, float deltaTime) = 0;
    [[nodiscard]] virtual bool IsTerminal() const = 0;
    [[nodiscard]] virtual float Score() const = 0;
};

struct ExperimentStatistics {
    std::uint32_t run{};
    std::uint32_t iteration{};
    float elapsedSeconds{};
    float iterationLimitSeconds{};
    float progress{};
    float bestScore{};
    float averageScore{};
    std::size_t population{};
    bool evaluating{};
    bool background{};
};

class Trainer {
  public:
    virtual ~Trainer() = default;
    virtual bool Start(std::string &errorMessage) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Stop() = 0;
    [[nodiscard]] virtual ExperimentStatistics Statistics() const = 0;
    virtual bool SaveCheckpoint(const std::filesystem::path &directory, std::string &errorMessage) const = 0;
    virtual bool LoadCheckpoint(const std::filesystem::path &directory, std::string &errorMessage) = 0;
};

} // namespace pipeframe::learning

#endif
