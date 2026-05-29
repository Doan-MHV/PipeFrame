#ifndef PIPEFRAME_NEWLEVELDIALOG_H
#define PIPEFRAME_NEWLEVELDIALOG_H

#include <string>

struct NewLevelResult
{
    bool requestedCreate = false;
    std::string levelName;
    int rows = 16;
    int cols = 16;
    int tileSize = 32;
    float scale = 2.0f;
};

class NewLevelDialog
{
private:
    char levelNameBuffer[128] = "Level2";
    int rows = 16;
    int cols = 16;
    int tileSize = 32;
    float scale = 2.0f;

public:
    void Open();
    NewLevelResult Draw();
};

#endif // PIPEFRAME_NEWLEVELDIALOG_H
