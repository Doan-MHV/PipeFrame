#include "PipeFrameQtTheme.h"

#include <QApplication>
#include <QFont>

void PipeFrameQtTheme::Apply(QApplication& app)
{
    QFont font = app.font();
    // font.setFamily("Inter");
    font.setPointSize(12);
    app.setFont(font);

    app.setStyleSheet(R"(
        QMainWindow {
            background: #2f3133;
            color: #f1f0ec;
        }

        QWidget#EditorRoot {
            background: #2f3133;
            color: #f1f0ec;
        }

        QFrame#TopBar,
        QFrame#Panel,
        QFrame#ViewportPanel,
        QFrame#ContentPanel {
            background: rgba(34, 35, 36, 232);
            border: 1px solid rgba(255, 255, 255, 34);
            border-radius: 10px;
        }

        QFrame#TopBar {
            border-radius: 0px;
            border-left: none;
            border-right: none;
            border-top: none;
        }

        QFrame#ViewportCanvas {
            background-color: #3f4040;
            border: 1px solid rgba(255, 255, 255, 24);
            border-radius: 8px;
        }

        QFrame#AssetArea {
            background: transparent;
            border: none;
        }

        QFrame#ContentToolbar {
            background: #1a1b1c;
            border: 1px solid rgba(255, 255, 255, 28);
            border-radius: 8px;
        }

        QLabel {
            color: #f1f0ec;
            background: transparent;
        }

        QLabel#PanelTitle {
            color: #f1f0ec;
            font-size: 13px;
            font-weight: 700;
        }

        QLabel#SectionTitle {
            color: #b9bbb9;
            font-size: 11px;
            font-weight: 700;
            letter-spacing: 0px;
            text-transform: uppercase;
        }

        QLabel#MutedText {
            color: #a6aaa8;
        }

        QLabel#ToolbarGroupLabel {
            color: #8f9492;
            font-size: 11px;
            font-weight: 800;
            padding-left: 2px;
            padding-right: 2px;
        }

        QFrame#ToolbarSeparator {
            background: rgba(255, 255, 255, 34);
            border: none;
            max-width: 1px;
            margin-left: 6px;
            margin-right: 6px;
        }

        QLabel#AxisX,
        QLabel#AxisY,
        QLabel#AxisZ {
            color: #ffffff;
            border-radius: 4px;
            padding: 6px 0px;
            font-size: 10px;
            font-weight: 800;
        }

        QLabel#AxisX {
            background: #d64b4b;
        }

        QLabel#AxisY {
            background: #45a85c;
        }

        QLabel#AxisZ {
            background: #3e78d8;
        }

        QWidget#AxisNumericCell {
            background: transparent;
            border: none;
        }

        QLabel#MetricValue {
            color: #61d0bd;
            font-size: 24px;
            font-weight: 800;
        }

        QLabel#ViewportTitle {
            color: #f1f0ec;
            font-size: 26px;
            font-weight: 800;
        }

        QLabel#ViewportHint {
            color: #b9bbb9;
            font-size: 13px;
        }

        QPushButton {
            background: #4a4c4d;
            color: #f1f0ec;
            border: 1px solid rgba(255, 255, 255, 42);
            border-radius: 8px;
            padding: 8px 13px;
            font-weight: 700;
        }

        QPushButton:hover {
            background: #585b5c;
            border-color: rgba(255, 255, 255, 76);
        }

        QPushButton:pressed {
            background: #3d3f40;
        }

        QPushButton#PrimaryButton {
            background: #f0ca66;
            color: #2b2b2c;
            border-color: #ffd978;
        }

        QPushButton#PrimaryButton:hover {
            background: #ffd978;
        }

        QPushButton#AccentButton {
            background: #4fc2ad;
            color: #1d2a29;
            border-color: #70dfcb;
        }

        QPushButton#DangerButton {
            background: #df5f74;
            color: #fff7f5;
            border-color: #f07b8d;
        }

        QPushButton#ChipButton {
            background: #3d3f40;
            border-radius: 7px;
            padding: 6px 11px;
            color: #dbddda;
        }

        QPushButton#ChipButton[active="true"] {
            background: #61d0bd;
            color: #1a2423;
            border-color: #7ce6d3;
        }

        QPushButton#AssetTile,
        QPushButton#ClassTile {
            background: #333536;
            color: #ececea;
            border: 1px solid rgba(255, 255, 255, 36);
            border-radius: 8px;
            padding: 10px;
            font-weight: 800;
            text-align: left;
        }

        QPushButton#AssetTile:hover,
        QPushButton#ClassTile:hover {
            border-color: #61d0bd;
            background: #3e4242;
        }

        QPushButton#ClassTile {
            background: rgba(79, 194, 173, 190);
            color: #14201f;
            border-color: #70dfcb;
        }

        QTreeWidget, QListWidget, QTableWidget {
            background: transparent;
            color: #f1f0ec;
            border: none;
            outline: none;
            selection-background-color: rgba(97, 208, 189, 72);
            selection-color: #ffffff;
            alternate-background-color: rgba(255, 255, 255, 10);
        }

        QTreeWidget::item,
        QListWidget::item {
            padding: 6px 8px;
            border-radius: 6px;
        }

        QTreeWidget::item:hover,
        QListWidget::item:hover {
            background: rgba(255, 255, 255, 18);
        }

        QHeaderView::section {
            background: transparent;
            color: #aeb2b0;
            border: none;
            padding: 6px;
            font-weight: 700;
        }

        QTreeWidget#FolderTree {
            background: #171819;
            border: 1px solid rgba(255, 255, 255, 24);
            border-radius: 8px;
            padding: 6px;
        }

        QLineEdit {
            background: #3b3d3e;
            color: #f1f0ec;
            border: 1px solid rgba(255, 255, 255, 34);
            border-radius: 7px;
            padding: 7px 9px;
        }

        QLineEdit#VectorInput {
            min-height: 18px;
            padding: 6px 7px;
            border-radius: 4px;
            background: #151617;
            border: 1px solid rgba(255, 255, 255, 30);
            font-size: 11px;
            font-weight: 700;
        }

        QDoubleSpinBox#VectorSpinBox {
            background: #111213;
            color: #f1f0ec;
            border: 1px solid rgba(255, 255, 255, 30);
            border-radius: 5px;
            padding: 6px 6px;
            font-size: 11px;
            font-weight: 800;
            selection-background-color: #61d0bd;
            selection-color: #14201f;
        }

        QLineEdit#PropertyInput,
        QComboBox#PropertyInput {
            background: #333536;
            border-radius: 6px;
            padding: 7px 9px;
            font-weight: 700;
        }

        QLineEdit#ContentSearch {
            background: #101112;
            border-radius: 8px;
            padding: 8px 12px;
        }

        QComboBox {
            background: #3b3d3e;
            color: #f1f0ec;
            border: 1px solid rgba(255, 255, 255, 34);
            border-radius: 7px;
            padding: 7px 9px;
        }

        QSplitter::handle {
            background: #252728;
        }

        QSplitter::handle:horizontal {
            width: 4px;
        }

        QSplitter::handle:vertical {
            height: 4px;
        }

        QMenuBar {
            background: #252728;
            color: #e6e6e3;
            border: none;
            padding: 4px;
        }

        QMenuBar::item {
            background: transparent;
            padding: 6px 10px;
            border-radius: 6px;
        }

        QMenuBar::item:selected {
            background: rgba(255, 255, 255, 22);
        }

        QMenu {
            background: #2d2f30;
            color: #f1f0ec;
            border: 1px solid rgba(255, 255, 255, 42);
            border-radius: 8px;
            padding: 6px;
        }

        QMenu::item {
            padding: 7px 28px 7px 12px;
            border-radius: 6px;
        }

        QMenu::item:selected {
            background: rgba(97, 208, 189, 56);
        }

        QStatusBar {
            background: #252728;
            color: #aeb2b0;
            border-top: 1px solid rgba(255, 255, 255, 28);
        }
    )");
}
