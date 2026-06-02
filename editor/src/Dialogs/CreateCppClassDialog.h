#ifndef PIPEFRAME_CREATECPPCLASSDIALOG_H
#define PIPEFRAME_CREATECPPCLASSDIALOG_H

#include <string>

#include "Core/EditorCommands.h"

struct CreateCppClassResult
{
    bool requestedCreate = false;
    CppClassKind kind = CppClassKind::Component;
    std::string className;
};

class CreateCppClassDialog
{
private:
    char classNameBuffer[128] = "MyComponent";
    int selectedKind = 0;

public:
    void Open();
    CreateCppClassResult Draw();
};

#endif // PIPEFRAME_CREATECPPCLASSDIALOG_H
