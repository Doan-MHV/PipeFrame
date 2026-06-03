#ifndef PIPEFRAME_EXAMPLEDENSESIMULATION_H
#define PIPEFRAME_EXAMPLEDENSESIMULATION_H

#include <SDL3/SDL.h>

#include "Assets/AssetRegistry.h"
#include "Simulation/DenseAgentSimulation.h"
#include "Simulation/ProjectRuntime.h"

struct ExampleAgent
{
    float x = 0.0f;
    float y = 0.0f;
};

class ExampleDenseSimulation : public DenseAgentSimulation<ExampleAgent>, public ProjectSimulation
{
public:
    void Start(ProjectRuntimeContext& context) override
    {
        (void)context;
        Clear();
    }

    void Update(ProjectRuntimeContext& context) override
    {
        (void)context;
    }

    void Render(SDL_Renderer* renderer, AssetRegistry& assetRegistry, const SDL_FRect& camera) override
    {
        (void)renderer;
        (void)assetRegistry;
        (void)camera;
    }
};

#endif // PIPEFRAME_EXAMPLEDENSESIMULATION_H
