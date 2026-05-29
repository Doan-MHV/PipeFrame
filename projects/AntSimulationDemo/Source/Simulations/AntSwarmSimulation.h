#ifndef PIPEFRAME_ANT_SWARM_SIMULATION_H
#define PIPEFRAME_ANT_SWARM_SIMULATION_H

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "Simulation/DenseAgentSimulation.h"
#include "Fields/FieldGrid.h"
#include "Simulations/AntAgent.h"
#include "Simulations/AntSwarmSettings.h"

class AntSwarmSimulation : public DenseAgentSimulation<AntAgent>
{
private:
    std::unordered_map<std::string, FieldGrid> fieldGrids;
    AntSwarmSettings settings;
    std::mt19937 randomEngine{1337};
    double spawnTimer = 0.0;
    int nextAgentId = 0;
    int selectedAgentId = -1;

public:
    void Reset()
    {
        Clear();
        fieldGrids.clear();
        spawnTimer = 0.0;
        nextAgentId = 0;
        selectedAgentId = -1;
        randomEngine.seed(1337);
    }

    std::unordered_map<std::string, FieldGrid>& Fields()
    {
        return fieldGrids;
    }

    const std::unordered_map<std::string, FieldGrid>& GetFields() const
    {
        return fieldGrids;
    }

    int AllocateAgentId()
    {
        return nextAgentId++;
    }

    AntSwarmSettings& Settings()
    {
        return settings;
    }

    const AntSwarmSettings& Settings() const
    {
        return settings;
    }

    std::mt19937& RandomEngine()
    {
        return randomEngine;
    }

    double& SpawnTimer()
    {
        return spawnTimer;
    }

    void SetSelectedAgentId(int id)
    {
        selectedAgentId = id;
    }

    int GetSelectedAgentId() const
    {
        return selectedAgentId;
    }
};

#endif // PIPEFRAME_ANT_SWARM_SIMULATION_H
