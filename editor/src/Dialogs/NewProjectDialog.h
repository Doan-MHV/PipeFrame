#ifndef PIPEFRAME_NEWPROJECTDIALOG_H
#define PIPEFRAME_NEWPROJECTDIALOG_H

#include <string>

struct NewProjectResult
{
    bool requestedCreate = false;
    std::string projectName;
    std::string parentDirectory;
    bool copySampleAntAssets = false;
};

class NewProjectDialog
{
private:
    char projectNameBuffer[128] = "AntSimulationDemo";
    char parentDirectoryBuffer[512] = "";
    bool copySampleAntAssets = false;

public:
    void Open();
    NewProjectResult Draw();
};

#endif // PIPEFRAME_NEWPROJECTDIALOG_H
