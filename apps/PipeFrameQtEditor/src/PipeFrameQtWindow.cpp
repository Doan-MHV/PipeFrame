#include "PipeFrameQtWindow.h"
#include "Widgets/DragDoubleSpinBox.h"

#include <QAction>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QPushButton>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace
{
void AddSectionTitle(QVBoxLayout* layout, const QString& title)
{
    auto* label = new QLabel(title);
    label->setObjectName("SectionTitle");
    layout->addWidget(label);
}

QLabel* BuildMutedLabel(const QString& text)
{
    auto* label = new QLabel(text);
    label->setObjectName("MutedText");
    return label;
}

void AddPropertyRow(QGridLayout* grid, int row, const QString& name, QWidget* editor)
{
    auto* label = BuildMutedLabel(name);
    grid->addWidget(label, row, 0);
    grid->addWidget(editor, row, 1);
}

QLineEdit* BuildInput(const QString& value)
{
    auto* input = new QLineEdit(value);
    input->setObjectName("PropertyInput");
    return input;
}

QLabel* BuildAxisChip(const QString& axis, const QString& objectName)
{
    auto* chip = new QLabel(axis);
    chip->setObjectName(objectName);
    chip->setAlignment(Qt::AlignCenter);
    chip->setFixedWidth(22);
    return chip;
}

QWidget* BuildAxisCell(const QString& axis, const QString& value, const QString& chipName)
{
    auto* cell = new QWidget();
    cell->setObjectName("AxisCell");

    auto* layout = new QHBoxLayout(cell);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);
    layout->addWidget(BuildAxisChip(axis, chipName));

    auto* input = BuildInput(value);
    input->setObjectName("VectorInput");
    input->setFixedWidth(58);
    layout->addWidget(input);

    return cell;
}

DragDoubleSpinBox* BuildNumberInput(double value, double min, double max, double step)
{
    auto* input = new DragDoubleSpinBox();
    input->setObjectName("VectorSpinBox");
    input->setRange(min, max);
    input->setSingleStep(step);
    input->setDecimals(2);
    input->setValue(value);
    input->setButtonSymbols(QAbstractSpinBox::NoButtons);
    input->setFixedWidth(74);
    return input;
}

QWidget* BuildAxisNumericCell(
    const QString& axis,
    double value,
    double min,
    double max,
    double step,
    const QString& chipName
)
{
    auto* cell = new QWidget();
    cell->setObjectName("AxisNumericCell");

    auto* layout = new QHBoxLayout(cell);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);

    auto* input = BuildNumberInput(value, min, max, step);
    layout->addWidget(BuildAxisChip(axis, chipName));
    layout->addWidget(input);
    return cell;
}

QWidget* BuildVectorPropertyRow(
    const QString& label,
    const QString& x,
    const QString& y,
    const QString& z
)
{
    auto* row = new QWidget();
    row->setObjectName("PropertyRow");

    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* nameLabel = BuildMutedLabel(label);
    nameLabel->setFixedWidth(88);
    layout->addWidget(nameLabel);

    const double xValue = x.toDouble();
    const double yValue = y.toDouble();
    const double zValue = z.toDouble();
    const bool isScale = label == "Scale";
    const double min = isScale ? 0.0 : -1000.0;
    const double max = isScale ? 10.0 : 1000.0;
    const double step = isScale ? 0.01 : 1.0;

    layout->addWidget(BuildAxisNumericCell("X", xValue, min, max, step, "AxisX"));
    layout->addWidget(BuildAxisNumericCell("Y", yValue, min, max, step, "AxisY"));
    layout->addWidget(BuildAxisNumericCell("Z", zValue, min, max, step, "AxisZ"));
    layout->addStretch();

    return row;
}

QWidget* BuildScalarPropertyRow(const QString& label, QWidget* editor)
{
    auto* row = new QWidget();
    row->setObjectName("PropertyRow");

    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* nameLabel = BuildMutedLabel(label);
    nameLabel->setFixedWidth(120);
    layout->addWidget(nameLabel);
    layout->addWidget(editor, 1);

    return row;
}

QLabel* BuildToolbarLabel(const QString& text)
{
    auto* label = new QLabel(text);
    label->setObjectName("ToolbarGroupLabel");
    return label;
}

QFrame* BuildToolbarSeparator()
{
    auto* separator = new QFrame();
    separator->setObjectName("ToolbarSeparator");
    separator->setFrameShape(QFrame::VLine);
    separator->setFixedHeight(28);
    return separator;
}
}

PipeFrameQtWindow::PipeFrameQtWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("PipeFrame Qt Editor");
    resize(1600, 980);
    ConfigureMenus();
    setCentralWidget(BuildEditorRoot());
    statusBar()->showMessage("Ready");
}

void PipeFrameQtWindow::ConfigureMenus()
{
    auto* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("New Project");
    fileMenu->addAction("Open Project");
    fileMenu->addSeparator();
    fileMenu->addAction("Save");
    fileMenu->addAction("Save All");

    auto* projectMenu = menuBar()->addMenu("Project");
    projectMenu->addAction("New Level");
    projectMenu->addAction("Project Settings");

    auto* buildMenu = menuBar()->addMenu("Build");
    buildMenu->addAction("Compile C++");
    buildMenu->addAction("Reload Module");

    auto* viewMenu = menuBar()->addMenu("View");
    viewMenu->addAction("Terrain");
    viewMenu->addAction("Fields");
    viewMenu->addAction("Colliders");
}

QWidget* PipeFrameQtWindow::BuildEditorRoot()
{
    auto* root = new QWidget();
    root->setObjectName("EditorRoot");

    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(10, 10, 10, 8);
    layout->setSpacing(8);
    layout->addWidget(BuildTopBar());

    auto* verticalSplitter = new QSplitter(Qt::Vertical);
    verticalSplitter->setChildrenCollapsible(false);
    verticalSplitter->addWidget(BuildMainArea());
    verticalSplitter->addWidget(BuildContentPanel());
    verticalSplitter->setStretchFactor(0, 7);
    verticalSplitter->setStretchFactor(1, 2);
    verticalSplitter->setSizes({720, 220});

    layout->addWidget(verticalSplitter, 1);
    return root;
}

QFrame* PipeFrameQtWindow::BuildTopBar()
{
    auto* topBar = new QFrame();
    topBar->setObjectName("TopBar");
    topBar->setMinimumHeight(74);

    auto* layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    layout->addWidget(BuildButton("Play", "PrimaryButton"));
    layout->addWidget(BuildButton("Stop", "DangerButton"));
    layout->addWidget(BuildToolbarSeparator());

    layout->addWidget(BuildToolbarLabel("Select"));
    layout->addWidget(BuildChip("Entity", true));
    layout->addWidget(BuildChip("Tile"));

    layout->addWidget(BuildToolbarSeparator());
    layout->addWidget(BuildToolbarLabel("Tile Edit"));
    layout->addWidget(BuildChip("Paint"));
    layout->addWidget(BuildChip("Erase"));
    layout->addWidget(BuildChip("Fill"));
    layout->addWidget(BuildChip("Pick"));

    layout->addWidget(BuildToolbarSeparator());
    layout->addWidget(BuildButton("Create C++ Class"));
    layout->addWidget(BuildButton("Compile C++", "AccentButton"));

    layout->addStretch();

    layout->addWidget(BuildToolbarLabel("View"));
    layout->addWidget(BuildChip("Terrain", true));
    layout->addWidget(BuildChip("Fields", true));
    layout->addWidget(BuildChip("Paths"));
    layout->addWidget(BuildChip("Colliders"));

    layout->addWidget(BuildToolbarSeparator());
    layout->addWidget(BuildButton("Save"));
    layout->addWidget(BuildButton("Save All"));

    return topBar;
}

QSplitter* PipeFrameQtWindow::BuildMainArea()
{
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(BuildHierarchyPanel());
    splitter->addWidget(BuildViewportPanel());
    splitter->addWidget(BuildInspectorPanel());
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({280, 900, 430});
    return splitter;
}

QFrame* PipeFrameQtWindow::BuildPanel(const QString& title)
{
    auto* panel = new QFrame();
    panel->setObjectName("Panel");

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 10, 12, 12);
    layout->setSpacing(8);

    auto* titleLabel = new QLabel(title);
    titleLabel->setObjectName("PanelTitle");
    layout->addWidget(titleLabel);

    return panel;
}

QFrame* PipeFrameQtWindow::BuildHierarchyPanel()
{
    auto* panel = BuildPanel("Hierarchy");
    auto* layout = qobject_cast<QVBoxLayout*>(panel->layout());

    auto* search = new QLineEdit();
    search->setPlaceholderText("Search entities");
    layout->addWidget(search);

    auto* tree = new QTreeWidget();
    tree->setHeaderHidden(true);
    tree->setAlternatingRowColors(true);
    tree->addTopLevelItem(new QTreeWidgetItem(QStringList{"colony_01  [colonies]"}));
    tree->addTopLevelItem(new QTreeWidgetItem(QStringList{"ant_swarm_01  [swarms]"}));
    tree->addTopLevelItem(new QTreeWidgetItem(QStringList{"food_01  [food]"}));
    tree->addTopLevelItem(new QTreeWidgetItem(QStringList{"marker_debug"}));
    tree->addTopLevelItem(new QTreeWidgetItem(QStringList{"camera"}));
    layout->addWidget(tree, 1);

    auto* row = new QHBoxLayout();
    row->setSpacing(6);
    row->addWidget(BuildButton("Create"));
    row->addWidget(BuildButton("Duplicate"));
    row->addWidget(BuildButton("Delete", "DangerButton"));
    layout->addLayout(row);

    return panel;
}

QFrame* PipeFrameQtWindow::BuildViewportPanel()
{
    auto* panel = new QFrame();
    panel->setObjectName("ViewportPanel");

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto* header = new QHBoxLayout();
    header->addWidget(new QLabel("Viewport"));
    header->addStretch();
    header->addWidget(BuildMetricCard("Simulation", "0.4 ms", "Update"));
    header->addWidget(BuildMetricCard("Render", "0.8 ms", "Frame"));
    layout->addLayout(header);

    auto* canvas = new QFrame();
    canvas->setObjectName("ViewportCanvas");
    canvas->setMinimumSize(480, 360);

    auto* canvasLayout = new QVBoxLayout(canvas);
    canvasLayout->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("PipeFrame Viewport");
    title->setObjectName("ViewportTitle");

    auto* hint = new QLabel("Engine viewport placeholder. The shell keeps the Unreal layout, but the skin is warmer and softer.");
    hint->setObjectName("ViewportHint");

    auto* controls = new QHBoxLayout();
    controls->setSpacing(8);
    controls->addStretch();
    controls->addWidget(BuildButton("0.5x"));
    controls->addWidget(BuildChip("1x", true));
    controls->addWidget(BuildButton("2x"));
    controls->addWidget(BuildButton("4x"));
    controls->addStretch();

    canvasLayout->addStretch();
    canvasLayout->addWidget(title, 0, Qt::AlignCenter);
    canvasLayout->addWidget(hint, 0, Qt::AlignCenter);
    canvasLayout->addSpacing(16);
    canvasLayout->addLayout(controls);
    canvasLayout->addStretch();

    layout->addWidget(canvas, 1);
    return panel;
}

QFrame* PipeFrameQtWindow::BuildInspectorPanel()
{
    auto* panel = BuildPanel("Inspector");
    auto* layout = qobject_cast<QVBoxLayout*>(panel->layout());

    AddSectionTitle(layout, "Selection");
    auto* identityGrid = new QGridLayout();
    identityGrid->setHorizontalSpacing(10);
    identityGrid->setVerticalSpacing(8);
    AddPropertyRow(identityGrid, 0, "Entity", new QLabel("ant_swarm_01"));
    AddPropertyRow(identityGrid, 1, "Tag", new QLabel("None"));
    AddPropertyRow(identityGrid, 2, "Group", new QLabel("swarms"));
    layout->addLayout(identityGrid);

    AddSectionTitle(layout, "Transform");
    layout->addWidget(BuildVectorPropertyRow("Location", "200.0", "160.0", "0.0"));
    layout->addWidget(BuildVectorPropertyRow("Rotation", "0.0", "0.0", "0.0"));
    layout->addWidget(BuildVectorPropertyRow("Scale", "1.0", "1.0", "1.0"));

    AddSectionTitle(layout, "Ant Swarm");
    layout->addWidget(BuildScalarPropertyRow("Max Agents", BuildInput("100")));
    layout->addWidget(BuildScalarPropertyRow("Speed", BuildInput("52.0")));
    layout->addWidget(BuildScalarPropertyRow("Explorer Ratio", BuildInput("0.10")));

    auto* collisionMode = new QComboBox();
    collisionMode->setObjectName("PropertyInput");
    collisionMode->addItems({"Soft", "Hard", "None"});
    layout->addWidget(BuildScalarPropertyRow("Collision", collisionMode));

    AddSectionTitle(layout, "Selected Tiles");
    layout->addWidget(BuildScalarPropertyRow("Count", BuildInput("24")));
    layout->addWidget(BuildScalarPropertyRow("Tile Asset", BuildInput("grass_01")));

    auto* terrainType = new QComboBox();
    terrainType->setObjectName("PropertyInput");
    terrainType->addItems({"Land", "Water", "Blocked"});
    layout->addWidget(BuildScalarPropertyRow("Terrain Type", terrainType));

    auto* movementType = new QComboBox();
    movementType->setObjectName("PropertyInput");
    movementType->addItems({"Land", "Water", "Air"});
    layout->addWidget(BuildScalarPropertyRow("Allowed Movement", movementType));

    auto* tileNote = BuildMutedLabel("Paint changes tile art. Terrain data changes selected tiles.");
    tileNote->setWordWrap(true);
    layout->addWidget(tileNote);

    layout->addStretch();
    return panel;
}

QFrame* PipeFrameQtWindow::BuildContentPanel()
{
    auto* panel = new QFrame();
    panel->setObjectName("ContentPanel");

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 10, 12, 12);
    layout->setSpacing(8);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel("Content Browser");
    title->setObjectName("PanelTitle");
    header->addWidget(title);
    header->addStretch();
    header->addWidget(BuildChip("Assets", true));
    header->addWidget(BuildChip("Prefabs"));
    header->addWidget(BuildChip("Classes"));
    header->addWidget(BuildChip("Levels"));
    layout->addLayout(header);

    auto* browser = new QSplitter(Qt::Horizontal);
    browser->setChildrenCollapsible(false);

    auto* folderTree = new QTreeWidget();
    folderTree->setObjectName("FolderTree");
    folderTree->setHeaderHidden(true);
    auto* projectRoot = new QTreeWidgetItem(QStringList{"AntSimulationDemo"});
    projectRoot->addChild(new QTreeWidgetItem(QStringList{"Content"}));
    projectRoot->addChild(new QTreeWidgetItem(QStringList{"Prefabs"}));
    projectRoot->addChild(new QTreeWidgetItem(QStringList{"Classes"}));
    projectRoot->addChild(new QTreeWidgetItem(QStringList{"Levels"}));
    folderTree->addTopLevelItem(projectRoot);
    folderTree->expandAll();
    browser->addWidget(folderTree);

    auto* assetArea = new QFrame();
    assetArea->setObjectName("AssetArea");
    auto* assetLayout = new QVBoxLayout(assetArea);
    assetLayout->setContentsMargins(10, 0, 0, 0);
    assetLayout->setSpacing(8);

    auto* toolbar = new QFrame();
    toolbar->setObjectName("ContentToolbar");
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 6, 8, 6);
    toolbarLayout->setSpacing(8);
    toolbarLayout->addWidget(BuildButton("+ Add", "AccentButton"));
    toolbarLayout->addWidget(BuildButton("Save All"));

    auto* search = new QLineEdit();
    search->setObjectName("ContentSearch");
    search->setPlaceholderText("Search Assets");
    toolbarLayout->addWidget(search, 1);
    toolbarLayout->addWidget(BuildChip("List", true));
    toolbarLayout->addWidget(BuildChip("Grid"));
    assetLayout->addWidget(toolbar);

    auto* contentGrid = new QGridLayout();
    contentGrid->setSpacing(8);
    const QStringList names = {
        "ant-texture",
        "marker-texture",
        "starter-tilemap",
        "AntSwarm",
        "Colony",
        "FoodSource"
    };

    int index = 0;
    for (const QString& name : names)
    {
        auto* card = new QPushButton(name);
        card->setObjectName(index < 3 ? "AssetTile" : "ClassTile");
        card->setMinimumSize(160, 64);
        contentGrid->addWidget(card, index / 4, index % 4);
        ++index;
    }

    assetLayout->addLayout(contentGrid);
    assetLayout->addStretch();
    browser->addWidget(assetArea);
    browser->setSizes({220, 1120});
    layout->addWidget(browser, 1);
    return panel;
}

QFrame* PipeFrameQtWindow::BuildMetricCard(const QString& title, const QString& value, const QString& detail)
{
    auto* card = new QFrame();
    card->setObjectName("Panel");
    card->setFixedWidth(120);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(0);

    auto* titleLabel = BuildMutedLabel(title);
    auto* valueLabel = new QLabel(value);
    valueLabel->setObjectName("MetricValue");
    auto* detailLabel = BuildMutedLabel(detail);

    layout->addWidget(titleLabel, 0, Qt::AlignCenter);
    layout->addWidget(valueLabel, 0, Qt::AlignCenter);
    layout->addWidget(detailLabel, 0, Qt::AlignCenter);
    return card;
}

QPushButton* PipeFrameQtWindow::BuildButton(const QString& text, const QString& objectName)
{
    auto* button = new QPushButton(text);
    button->setCursor(Qt::PointingHandCursor);
    if (!objectName.isEmpty())
    {
        button->setObjectName(objectName);
    }
    return button;
}

QPushButton* PipeFrameQtWindow::BuildChip(const QString& text, bool active)
{
    auto* button = BuildButton(text, "ChipButton");
    button->setProperty("active", active ? "true" : "false");
    return button;
}
