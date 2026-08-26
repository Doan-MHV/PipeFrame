#include "WorkbenchScene.h"

#include <PipeFrame/Core/Application.h>

int main() {
    Application app(1600, 1200, "PipeFrame - Simulation Workbench");

    app.SetScene(CreateSimulationWorkbenchScene());

    app.Run();

    return 0;
}