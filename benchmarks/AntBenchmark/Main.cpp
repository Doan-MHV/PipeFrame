#include <memory>

#include <PipeFrame/Core/Application.h>

#include "AntBenchmarkScene.h"

int main() {
    Application application(1280, 720, "PipeFrame Ant Benchmark");

    application.SetScene(std::make_unique<AntBenchmarkScene>());

    application.Run();

    return 0;
}
