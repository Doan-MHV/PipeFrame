#include "DragDoubleSpinBox.h"

#include <QApplication>
#include <QMouseEvent>

DragDoubleSpinBox::DragDoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
{
    setCursor(Qt::SizeHorCursor);
}

void DragDoubleSpinBox::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragStartPosition = event->position().toPoint();
        dragStartValue = value();
        dragActive = false;
    }

    QDoubleSpinBox::mousePressEvent(event);
}

void DragDoubleSpinBox::mouseMoveEvent(QMouseEvent* event)
{
    if ((event->buttons() & Qt::LeftButton) == 0)
    {
        QDoubleSpinBox::mouseMoveEvent(event);
        return;
    }

    const int dragDistance = event->position().toPoint().x() - dragStartPosition.x();
    if (!dragActive && qAbs(dragDistance) < QApplication::startDragDistance())
    {
        QDoubleSpinBox::mouseMoveEvent(event);
        return;
    }

    dragActive = true;

    double multiplier = 1.0;
    if ((event->modifiers() & Qt::ShiftModifier) != 0)
    {
        multiplier = 10.0;
    }
    else if ((event->modifiers() & Qt::AltModifier) != 0)
    {
        multiplier = 0.1;
    }

    setValue(dragStartValue + static_cast<double>(dragDistance) * singleStep() * multiplier);
    event->accept();
}

void DragDoubleSpinBox::mouseReleaseEvent(QMouseEvent* event)
{
    if (dragActive)
    {
        dragActive = false;
        event->accept();
        return;
    }

    QDoubleSpinBox::mouseReleaseEvent(event);
}
