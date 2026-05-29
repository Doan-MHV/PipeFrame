#ifndef PIPEFRAME_DENSEAGENTSIMULATION_H
#define PIPEFRAME_DENSEAGENTSIMULATION_H

#include <cstddef>
#include <vector>

template <typename TAgent>
class DenseAgentSimulation
{
private:
    std::vector<TAgent> agents;

public:
    std::vector<TAgent>& Agents()
    {
        return agents;
    }

    const std::vector<TAgent>& Agents() const
    {
        return agents;
    }

    void Reserve(std::size_t count)
    {
        agents.reserve(count);
    }

    void Resize(std::size_t count)
    {
        agents.resize(count);
    }

    void Clear()
    {
        agents.clear();
    }

    std::size_t Count() const
    {
        return agents.size();
    }
};

#endif // PIPEFRAME_DENSEAGENTSIMULATION_H
