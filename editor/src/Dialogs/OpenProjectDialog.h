#ifndef PIPEFRAME_OPENPROJECTDIALOG_H
#define PIPEFRAME_OPENPROJECTDIALOG_H

#include <string>

struct OpenProjectResult
{
    bool requestedOpen = false;
    std::string projectFilePath;
};

class OpenProjectDialog
{
private:
    char projectFilePathBuffer[512] = "projects/AntSimulationDemo/PipeFrameProject.json";

public:
    void Open();
    OpenProjectResult Draw();
};

#endif // PIPEFRAME_OPENPROJECTDIALOG_H
