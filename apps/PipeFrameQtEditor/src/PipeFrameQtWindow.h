#ifndef PIPEFRAME_QT_WINDOW_H
#define PIPEFRAME_QT_WINDOW_H

#include <QMainWindow>

class QFrame;
class QPushButton;
class QSplitter;
class QWidget;

class PipeFrameQtWindow final : public QMainWindow
{
public:
    explicit PipeFrameQtWindow(QWidget* parent = nullptr);

private:
    QWidget* BuildEditorRoot();
    QFrame* BuildTopBar();
    QSplitter* BuildMainArea();
    QFrame* BuildHierarchyPanel();
    QFrame* BuildViewportPanel();
    QFrame* BuildInspectorPanel();
    QFrame* BuildContentPanel();

    QFrame* BuildPanel(const QString& title);
    QFrame* BuildMetricCard(const QString& title, const QString& value, const QString& detail);
    QPushButton* BuildButton(const QString& text, const QString& objectName = QString());
    QPushButton* BuildChip(const QString& text, bool active = false);

    void ConfigureMenus();
};

#endif // PIPEFRAME_QT_WINDOW_H
