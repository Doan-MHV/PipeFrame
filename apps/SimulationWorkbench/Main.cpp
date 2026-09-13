#include "Backend/SFML/Workbench.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include <PipeFrame/Backend/SFML/Core/Application.h>

namespace {

void PrintUsage() {
    std::cout
        << "Usage:\n"
        << "  SimulationWorkbench\n"
        << "  SimulationWorkbench --project "
           "<project-directory-or-manifest>\n";
}

} // namespace

int main(
    const int argumentCount,
    char **argumentValues) {

    std::optional<std::filesystem::path>
        startupProject;

    if (argumentCount == 3 &&
        std::string(argumentValues[1]) ==
            "--project") {

        startupProject =
            std::filesystem::path(
                argumentValues[2]);
            } else if (argumentCount != 1) {
                PrintUsage();
                return 1;
            }

    Application application(
        1600,
        1200,
        "PipeFrame - Simulation Workbench");

    application.SetScene(
        CreateWorkbench(
            std::move(startupProject)));

    application.Run();

    return 0;
}