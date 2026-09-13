#include "Editor/AntEditorTool.h"

#include <algorithm>

#include "World/Runtime/Environment/AntWorldCell.h"

namespace ant_simulation {

void AntEditorTool::SetEnabled(const bool newEnabled) {
    enabled = newEnabled;

    if (!enabled) {
        strokeActive = false;
        hasAppliedPosition = false;
        pendingPreviewCells.clear();
    }
}

void AntEditorTool::SetMode(const AntEditorToolMode newMode) {
    if (mode == newMode) {
        return;
    }

    mode = newMode;
    strokeActive = false;
    hasAppliedPosition = false;
    pendingPreviewCells.clear();
}

void AntEditorTool::SetRadius(const float newRadius) { radius = std::clamp(newRadius, MinimumRadius, MaximumRadius); }

void AntEditorTool::SetFoodQuantity(const std::size_t quantity) { foodQuantity = std::max<std::size_t>(1, quantity); }

void AntEditorTool::SetPosition(const pipeframe::Vector2f newPosition) { position = newPosition; }

std::size_t AntEditorTool::BeginStroke(AntEnvironment &environment) {
    if (!enabled || mode == AntEditorToolMode::None) {
        return 0;
    }

    strokeActive = true;
    hasAppliedPosition = false;
    pendingPreviewCells.clear();

    return Apply(environment);
}

std::size_t AntEditorTool::UpdateStroke(AntEnvironment &environment) {
    if (!enabled || !strokeActive || mode == AntEditorToolMode::None) {
        return 0;
    }

    if (hasAppliedPosition && lastAppliedPosition == position) {
        return 0;
    }

    return Apply(environment);
}

std::size_t AntEditorTool::EndStroke(AntEnvironment &environment) {
    if (!strokeActive) {
        return 0;
    }

    strokeActive = false;
    hasAppliedPosition = false;

    std::size_t changedCellCount{0};

    switch (mode) {
    case AntEditorToolMode::AddWall:
        changedCellCount = environment.ApplyWallRequests();
        break;

    case AntEditorToolMode::Erase:
        changedCellCount = environment.ApplyEraseRequests();
        break;

    case AntEditorToolMode::AddFood:
    case AntEditorToolMode::None:
        break;
    }

    pendingPreviewCells.clear();

    return changedCellCount;
}

void AntEditorTool::CancelStroke(AntEnvironment &environment) {
    if (!strokeActive && pendingPreviewCells.empty()) {
        return;
    }

    ClearPendingRequests(environment);

    strokeActive = false;
    hasAppliedPosition = false;
    pendingPreviewCells.clear();
}

void AntEditorTool::CycleMode() {
    switch (mode) {
    case AntEditorToolMode::AddFood:
        SetMode(AntEditorToolMode::AddWall);
        break;

    case AntEditorToolMode::AddWall:
        SetMode(AntEditorToolMode::Erase);
        break;

    case AntEditorToolMode::Erase:
    case AntEditorToolMode::None:
        SetMode(AntEditorToolMode::AddFood);
        break;
    }
}

bool AntEditorTool::IsEnabled() const { return enabled; }

bool AntEditorTool::IsStrokeActive() const { return strokeActive; }

AntEditorToolMode AntEditorTool::GetMode() const { return mode; }

float AntEditorTool::GetRadius() const { return radius; }

std::size_t AntEditorTool::GetFoodQuantity() const { return foodQuantity; }

pipeframe::Vector2f AntEditorTool::GetPosition() const { return position; }

std::span<const pipeframe::Vector2f> AntEditorTool::GetPendingPreviewCells() const { return pendingPreviewCells; }

const char *AntEditorTool::GetModeName(const AntEditorToolMode requestedMode) {
    switch (requestedMode) {
    case AntEditorToolMode::None:
        return "None";

    case AntEditorToolMode::Erase:
        return "Eraser";

    case AntEditorToolMode::AddFood:
        return "Add Food";

    case AntEditorToolMode::AddWall:
        return "Add Wall";
    }

    return "None";
}

std::size_t AntEditorTool::Apply(AntEnvironment &environment) {
    if (!environment.IsInitialized()) {
        return 0;
    }

    lastAppliedPosition = position;
    hasAppliedPosition = true;

    std::size_t changedCellCount{0};

    ForEachBrushCell(environment, [this, &environment, &changedCellCount](const pipeframe::Vector2f cellCenter) {
        switch (mode) {
        case AntEditorToolMode::AddFood:
            if (environment.AddFood(cellCenter, foodQuantity) != InvalidWorldEntityId) {
                ++changedCellCount;
            }
            break;

        case AntEditorToolMode::AddWall:
            if (environment.MarkCellForWall(cellCenter)) {
                pendingPreviewCells.push_back(cellCenter);

                ++changedCellCount;
            }
            break;

        case AntEditorToolMode::Erase:
            if (environment.MarkCellForErase(cellCenter)) {
                pendingPreviewCells.push_back(cellCenter);

                ++changedCellCount;
            }
            break;

        case AntEditorToolMode::None:
            break;
        }
    });

    return changedCellCount;
}

void AntEditorTool::ClearPendingRequests(AntEnvironment &environment) {
    for (const pipeframe::Vector2f cellPosition : pendingPreviewCells) {
        AntWorldCell *cell = environment.TryGetCellAtWorldPosition(cellPosition);

        if (cell != nullptr) {
            cell->ResetEditState();
        }
    }
}

} // namespace ant_simulation