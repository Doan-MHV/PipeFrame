#include "Configuration/AntConfiguration.h"
#include "Editor/AntEditorTool.h"
#include "Editor/AntEditorToolRenderer.h"
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
        16,
        16,
    };

    configuration.colonyPosition = {
        8.0f,
        8.0f,
    };

    AntEnvironment environment;
    std::string errorMessage;

    Require(environment.Initialize(configuration, errorMessage), "Environment should initialize.");

    AntEditorTool tool;

    tool.SetEnabled(true);
    tool.SetMode(AntEditorToolMode::AddFood);

    tool.SetRadius(1.5f);

    tool.SetPosition({
        8.5f,
        8.5f,
    });

    AntEditorToolRenderer renderer(configuration);

    const pipeframe::Rectanglef viewport{
        {0.0f, 0.0f},
        {16.0f, 16.0f},
    };

    renderer.Update(tool, environment, viewport);

    Require(renderer.GetVisibleBrushCellCount() == 9, "Radius 1.5 preview should contain nine cells.");

    Require(renderer.GetBrushVertices().size() == 54, "Nine preview cells should use 54 vertices.");

    const pipeframe::Color foodPreviewColor = renderer.GetPreviewColor(AntEditorToolMode::AddFood);

    Require(foodPreviewColor.r == configuration.foodColor.r && foodPreviewColor.g == configuration.foodColor.g &&
                foodPreviewColor.b == configuration.foodColor.b &&
                foodPreviewColor.a == AntEditorToolRenderer::PreviewAlpha,
            "Food preview should use translucent food color.");

    tool.SetMode(AntEditorToolMode::AddWall);

    tool.SetRadius(0.6f);

    tool.SetPosition({
        10.5f,
        10.5f,
    });

    renderer.Update(tool, environment, viewport);

    Require(renderer.GetVisibleBrushCellCount() == 1, "Small wall brush should preview one cell.");

    Require(renderer.GetBrushVertices()[0].color == AntEditorToolRenderer::WallPreviewColor,
            "Wall preview should use the blue wall color.");

    Require(tool.BeginStroke(environment) == 1, "Wall stroke should mark one cell.");

    renderer.Update(tool, environment, viewport);

    Require(renderer.GetVisiblePendingCellCount() == 1, "Pending wall should remain visible.");

    Require(renderer.GetPendingVertices().size() == 6, "One pending wall should use six vertices.");

    tool.CancelStroke(environment);

    renderer.Update(tool, environment, viewport);

    Require(renderer.GetVisiblePendingCellCount() == 0, "Cancelling should remove pending preview.");

    tool.SetMode(AntEditorToolMode::Erase);

    renderer.Update(tool, environment, viewport);

    Require(renderer.GetBrushVertices()[0].color == AntEditorToolRenderer::ErasePreviewColor,
            "Erase preview should be translucent red.");

    const pipeframe::Rectanglef hiddenViewport{
        {100.0f, 100.0f},
        {10.0f, 10.0f},
    };

    renderer.Update(tool, environment, hiddenViewport);

    Require(renderer.GetVisibleBrushCellCount() == 0, "Off-screen brush cells should be culled.");

    Require(renderer.GetBrushVertices().empty(), "Culled brush should contain no geometry.");

    tool.SetEnabled(false);

    renderer.Update(tool, environment, viewport);

    Require(renderer.GetBrushVertices().empty(), "Disabled editor tool should have no preview.");

    Require(renderer.GetPendingVertices().empty(), "Disabled editor tool should have no pending preview.");

    std::cout << "All ant editor tool renderer tests passed.\n";

    return 0;
}
