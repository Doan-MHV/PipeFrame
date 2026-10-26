#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
namespace pipeframe {
struct ProcessCommand {
    std::string label;
    std::vector<std::string> arguments;
};
struct ProcessProgress {
    bool running{}, cancelled{};
    int exitCode{};
    std::string stage, output;
};
// Runs argument vectors without a shell. Cancellation terminates the process group.
class ProcessTask {
public:
    ProcessTask();
    ~ProcessTask();
    ProcessTask(const ProcessTask&) = delete;
    ProcessTask& operator=(const ProcessTask&) = delete;
    bool Start(std::vector<ProcessCommand> commands, const std::filesystem::path& directory,
               const std::filesystem::path& log, std::string& error);
    void Cancel();
    ProcessProgress Poll() const;

private:
    struct State;
    std::unique_ptr<State> state;
};
}  // namespace pipeframe
