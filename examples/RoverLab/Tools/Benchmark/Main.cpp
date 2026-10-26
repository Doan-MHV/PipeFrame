#include <csignal>
#include <iostream>
#include <stdexcept>

#include "Benchmark.h"

namespace {
volatile std::sig_atomic_t interrupted = 0;
void Interrupt(int) {
  interrupted = 1;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    std::filesystem::path project = ROVER_PROJECT_ROOT;
    std::filesystem::path plan, output;
    for (int i = 1; i < argc; ++i) {
      const std::string option = argv[i];
      if (option == "--help") {
        std::cout
          << "RoverLabBenchmark --plan FILE --output NEW_DIRECTORY [--project DIRECTORY]\n"
             "Runs saved scenes at 60 fixed steps/s. Results and input snapshots are preserved.\n"
             "Exit 0: expected objectives and repeatability passed; 1: recorded unsuccessful "
             "runs;\n"
             "2: invalid invocation, plan, or output error. Ctrl-C records cancellation.\n";
        return 0;
      }
      if (++i >= argc) throw std::runtime_error("Missing value for " + option);
      if (option == "--plan")
        plan = argv[i];
      else if (option == "--output")
        output = argv[i];
      else if (option == "--project")
        project = argv[i];
      else
        throw std::runtime_error("Unknown option: " + option);
    }
    if (plan.empty() || output.empty())
      throw std::runtime_error("Use --plan and --output; see --help");
    std::signal(SIGINT, Interrupt);
    std::signal(SIGTERM, Interrupt);
    return RoverLab::benchmark::RunBatch(project, plan, output, [] { return interrupted != 0; });
  } catch (const std::exception& error) {
    std::cerr << "Benchmark: " << error.what() << '\n';
    return 2;
  }
}
