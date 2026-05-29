#include "FieldGridSolver.h"

#include <algorithm>
#include <cmath>

#include "Map/TileMap.h"
#include "Project/ProjectConfig.h"

void FieldGridSolver::EnsureMatchesTileMap(
    FieldGrid& fieldGrid,
    const TileMap& tileMap,
    const SimulationConfig& simulationConfig
)
{
    const float mapWidth = static_cast<float>(tileMap.GetWorldWidth());
    const float mapHeight = static_cast<float>(tileMap.GetWorldHeight());
    const float cellWorldSize = std::max(simulationConfig.fieldCellSize, 1.0f);
    const int rows = std::max(static_cast<int>(std::ceil(mapHeight / cellWorldSize)), 1);
    const int cols = std::max(static_cast<int>(std::ceil(mapWidth / cellWorldSize)), 1);
    const std::size_t cellCount = static_cast<std::size_t>(rows) *
        static_cast<std::size_t>(cols);

    if (fieldGrid.rows == rows &&
        fieldGrid.cols == cols &&
        fieldGrid.cellWorldSize == cellWorldSize &&
        fieldGrid.values.size() == cellCount)
    {
        return;
    }

    fieldGrid.rows = rows;
    fieldGrid.cols = cols;
    fieldGrid.cellWorldSize = cellWorldSize;
    fieldGrid.values.assign(cellCount, 0.0);
}

void FieldGridSolver::Decay(
    FieldGrid& fieldGrid,
    double deltaTime,
    const SimulationConfig& simulationConfig
)
{
    const double decayFactor = std::clamp(
        1.0 - simulationConfig.fieldDecayPerSecond * deltaTime,
        0.0,
        1.0
    );

    for (double& value : fieldGrid.values)
    {
        value *= decayFactor;

        if (std::abs(value) < 0.0001)
        {
            value = 0.0;
        }
    }
}

void FieldGridSolver::ApplyDeposit(
    FieldGrid& fieldGrid,
    const TileMap& tileMap,
    const SimulationConfig& simulationConfig,
    const FieldDepositCommand& command
)
{
    EnsureMatchesTileMap(fieldGrid, tileMap, simulationConfig);

    if (fieldGrid.cellWorldSize <= 0.0f || fieldGrid.rows <= 0 || fieldGrid.cols <= 0)
    {
        return;
    }

    const double radius = std::max(command.radius, 0.0);
    const int startCol = static_cast<int>(std::floor((command.x - radius) / fieldGrid.cellWorldSize));
    const int endCol = static_cast<int>(std::floor((command.x + radius) / fieldGrid.cellWorldSize));
    const int startRow = static_cast<int>(std::floor((command.y - radius) / fieldGrid.cellWorldSize));
    const int endRow = static_cast<int>(std::floor((command.y + radius) / fieldGrid.cellWorldSize));

    for (int row = std::max(startRow, 0); row <= std::min(endRow, fieldGrid.rows - 1); row++)
    {
        for (int col = std::max(startCol, 0); col <= std::min(endCol, fieldGrid.cols - 1); col++)
        {
            const double cellCenterX = (static_cast<double>(col) + 0.5) * fieldGrid.cellWorldSize;
            const double cellCenterY = (static_cast<double>(row) + 0.5) * fieldGrid.cellWorldSize;
            const double distance = std::hypot(cellCenterX - command.x, cellCenterY - command.y);

            if (radius > 0.0 && distance > radius)
            {
                continue;
            }

            double weight = 1.0;
            if (radius > 0.0 && command.falloff == "linear")
            {
                weight = std::clamp(1.0 - (distance / radius), 0.0, 1.0);
            }

            if (weight <= 0.0)
            {
                continue;
            }

            const std::size_t index = static_cast<std::size_t>(row) *
                static_cast<std::size_t>(fieldGrid.cols) +
                static_cast<std::size_t>(col);

            if (index >= fieldGrid.values.size())
            {
                continue;
            }

            fieldGrid.values[index] = std::clamp(
                fieldGrid.values[index] + command.amount * weight,
                0.0,
                1000.0
            );
        }
    }
}

nlohmann::json FieldGridSolver::BuildFieldViews(
    const std::unordered_map<std::string, FieldGrid>& fieldGrids
)
{
    nlohmann::json fieldsJson = nlohmann::json::object();

    for (const auto& [name, fieldGrid] : fieldGrids)
    {
        fieldsJson[name] = {
            {"rows", fieldGrid.rows},
            {"cols", fieldGrid.cols},
            {"cellWorldSize", fieldGrid.cellWorldSize},
            {"values", fieldGrid.values}
        };
    }

    return fieldsJson;
}

nlohmann::json FieldGridSolver::BuildFieldMetadataViews(
    const std::unordered_map<std::string, FieldGrid>& fieldGrids
)
{
    nlohmann::json fieldsJson = nlohmann::json::object();

    for (const auto& [name, fieldGrid] : fieldGrids)
    {
        fieldsJson[name] = {
            {"rows", fieldGrid.rows},
            {"cols", fieldGrid.cols},
            {"cellWorldSize", fieldGrid.cellWorldSize}
        };
    }

    return fieldsJson;
}
