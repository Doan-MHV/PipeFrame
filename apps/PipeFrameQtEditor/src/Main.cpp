#include "PipeFrameQtTheme.h"
#include "PipeFrameQtWindow.h"

#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    PipeFrameQtTheme::Apply(app);

    PipeFrameQtWindow window;
    window.show();

    return app.exec();
}
