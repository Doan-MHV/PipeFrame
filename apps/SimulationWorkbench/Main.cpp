#include "WorkbenchScene.h"

#include <PipeFrame/Core/Application.h>

int main() {
    Application app(1920, 1080, "PipeFrame - Simulation Workbench");

    app.SetScene(CreateSimulationWorkbenchScene());

    app.Run();

    return 0;
}