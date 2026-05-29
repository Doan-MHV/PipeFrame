#include "App/PipeFrameApplication.h"
#include "App/EditorSettings.h"

#include <filesystem>

#ifndef PIPEFRAME_DEFAULT_PROJECT_FILE
#define PIPEFRAME_DEFAULT_PROJECT_FILE "projects/JungleDemo/PipeFrameProject.json"
#endif

int main(int argc, char* argv[])
{
    const std::filesystem::path projectFilePath = ResolveStartupProjectFile(
        argc,
        argv,
        PIPEFRAME_DEFAULT_PROJECT_FILE
    );

    PipeFrameApplication application(projectFilePath);

    if (!application.Initialize())
    {
        application.Shutdown();
        return 1;
    }

    application.Run();
    application.Shutdown();

    return 0;
}
