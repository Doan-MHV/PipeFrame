#ifndef PIPEFRAME_FIELDGRIDSOLVER_H
#define PIPEFRAME_FIELDGRIDSOLVER_H

#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "Fields/FieldGrid.h"

struct SimulationConfig;
class TileMap;

struct FieldDepositCommand
{
    std::string fieldName;
    double x = 0.0;
    double y = 0.0;
    double amount = 0.0;
    double radius = 0.0;
    std::string falloff = "none";
};

class FieldGridSolver
{
public:
    static void EnsureMatchesTileMap(
        FieldGrid& fieldGrid,
        const TileMap& tileMap,
        const SimulationConfig& simulationConfig
    );
    static void Decay(
        FieldGrid& fieldGrid,
        double deltaTime,
        const SimulationConfig& simulationConfig
    );
    static void ApplyDeposit(
        FieldGrid& fieldGrid,
        const TileMap& tileMap,
        const SimulationConfig& simulationConfig,
        const FieldDepositCommand& command
    );
    static nlohmann::json BuildFieldViews(
        const std::unordered_map<std::string, FieldGrid>& fieldGrids
    );
    static nlohmann::json BuildFieldMetadataViews(
        const std::unordered_map<std::string, FieldGrid>& fieldGrids
    );
};

#endif // PIPEFRAME_FIELDGRIDSOLVER_H
