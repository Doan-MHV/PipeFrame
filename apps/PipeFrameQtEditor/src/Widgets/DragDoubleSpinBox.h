#ifndef PIPEFRAME_DRAG_DOUBLE_SPIN_BOX_H
#define PIPEFRAME_DRAG_DOUBLE_SPIN_BOX_H

#include <QDoubleSpinBox>
#include <QPoint>

class DragDoubleSpinBox final : public QDoubleSpinBox
{
public:
    explicit DragDoubleSpinBox(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QPoint dragStartPosition;
    double dragStartValue = 0.0;
    bool dragActive = false;
};

#endif // PIPEFRAME_DRAG_DOUBLE_SPIN_BOX_H
