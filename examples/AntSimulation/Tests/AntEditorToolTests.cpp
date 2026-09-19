#include "Configuration/AntConfiguration.h"
#include "Editor/AntEditorTool.h"
#include "World/Runtime/Environment/AntEnvironment.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Require(const bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';

        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace ant_simulation;

    AntConfiguration configuration;

    configuration.worldSize = {
        32,
        32,
    };

    configuration.colonyPosition = {
        16.0f,
        16.0f,
    };

    AntEnvironment environment;
    std::string errorMessage;

    Require(environment.Initialize(configuration, errorMessage), "Environment should initialize.");

    AntEditorTool tool;

    tool.SetEnabled(true);
    tool.SetMode(AntEditorToolMode::AddFood);

    tool.SetRadius(1.5f);
    tool.SetFoodQuantity(10);

    tool.SetPosition({
        10.5f,
        10.5f,
    });

    const std::size_t foodCells = tool.BeginStroke(environment);

    Require(foodCells == 9, "A radius 1.5 food brush should affect nine cells.");

    Require(environment.GetTotalFoodQuantity() == 90, "Food brush should add ten food to each affected cell.");

    Require(tool.UpdateStroke(environment) == 0, "A stationary brush should not apply repeatedly.");

    tool.SetPosition({
        12.5f,
        10.5f,
    });

    Require(tool.UpdateStroke(environment) > 0, "Moving the brush should apply it again.");

    tool.EndStroke(environment);

    tool.SetMode(AntEditorToolMode::AddWall);

    tool.SetRadius(0.6f);

    tool.SetPosition({
        20.5f,
        20.5f,
    });

    Require(tool.BeginStroke(environment) == 1, "Wall brush should mark one cell.");

    Require(tool.GetPendingPreviewCells().size() == 1, "Wall brush should retain preview geometry.");

    const AntWorldCell *wallCell = environment.TryGetCellAtWorldPosition({
        20.5f,
        20.5f,
    });

    Require(wallCell != nullptr && !wallCell->wall, "Wall should remain pending until stroke ends.");

    Require(tool.EndStroke(environment) == 1, "Ending wall stroke should commit one wall.");

    wallCell = environment.TryGetCellAtWorldPosition({
        20.5f,
        20.5f,
    });

    Require(wallCell != nullptr && wallCell->wall, "Committed wall should exist.");

    Require(tool.GetPendingPreviewCells().empty(), "Committed preview should be cleared.");

    tool.SetMode(AntEditorToolMode::Erase);

    tool.SetPosition({
        20.5f,
        20.5f,
    });

    Require(tool.BeginStroke(environment) == 1, "Erase brush should mark the wall.");

    Require(tool.EndStroke(environment) == 1, "Ending erase stroke should remove the wall.");

    wallCell = environment.TryGetCellAtWorldPosition({
        20.5f,
        20.5f,
    });

    Require(wallCell != nullptr && !wallCell->wall, "Erase brush should remove committed wall.");

    tool.SetMode(AntEditorToolMode::AddWall);

    tool.SetPosition({
        22.5f,
        22.5f,
    });

    Require(tool.BeginStroke(environment) == 1, "Another wall should become pending.");

    tool.CancelStroke(environment);

    const AntWorldCell *cancelledCell = environment.TryGetCellAtWorldPosition({
        22.5f,
        22.5f,
    });

    Require(cancelledCell != nullptr && !cancelledCell->wall && cancelledCell->editState == WorldCellEditState::None,
            "Cancelling should discard pending wall request.");

    tool.SetMode(AntEditorToolMode::AddFood);

    tool.CycleMode();

    Require(tool.GetMode() == AntEditorToolMode::AddWall, "Cycle should move from food to wall.");

    tool.CycleMode();

    Require(tool.GetMode() == AntEditorToolMode::Erase, "Cycle should move from wall to erase.");

    tool.CycleMode();

    Require(tool.GetMode() == AntEditorToolMode::AddFood, "Cycle should return from erase to food.");

    tool.SetEnabled(false);

    Require(tool.BeginStroke(environment) == 0, "Disabled tool should not modify the environment.");

    Require(std::string{AntEditorTool::GetModeName(AntEditorToolMode::AddFood)} == "Add Food",
            "Food mode should have a readable name.");

    std::cout << "All ant editor tool tests passed.\n";

    return 0;
}
