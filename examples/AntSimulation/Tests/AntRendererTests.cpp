#include "World/Runtime/AntQuery.h"
#include "Configuration/AntConfiguration.h"
#include "World/Rendering/AntRenderer.h"

#include <cstdlib>
#include <iostream>

namespace {

void Require(
    const bool condition,
    const char *message
) {
    if (!condition) {
        std::cerr
            << "FAILED: "
            << message
            << '\n';

        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace ant_simulation;

    AntConfiguration configuration;
    pipeframe::BehaviourScene antStoreScene;
    AntQuery antStore(antStoreScene);

    antStore.Create(
        1,
        AntRole::Follower,
        {10.0f, 10.0f},
        0.0f,
        0.0f,
        configuration);

    antStore.Create(
        1,
        AntRole::Follower,
        {100.0f, 100.0f},
        0.0f,
        0.0f,
        configuration);

    const pipeframe::Rectanglef viewport{
        {0.0f, 0.0f},
        {20.0f, 20.0f},
    };

    AntRenderer renderer(
        configuration);

    renderer.UpdateGeometry(
        antStore.GetAnts(),
        viewport,
        20.0f);

    Require(
        renderer.GetMode() ==
            AntRenderingMode::
                DetailedQuads,
        "Close zoom should use detailed ant geometry.");

    Require(
        renderer.GetStatistics()
                .candidateCount ==
            2,
        "Renderer should report all candidates.");

    Require(
        renderer.GetStatistics()
                .visibleCount ==
            1,
        "Renderer should cull the off-screen ant.");

    Require(
        renderer.GetGeometry()
                .GetBodyVertices()
                .size() ==
            18,
        "One detailed ant should use three body quads.");

    renderer.UpdateGeometry(
        antStore.GetAnts(),
        viewport,
        5.0f);

    Require(
        renderer.GetMode() ==
            AntRenderingMode::
                SimpleQuads,
        "Medium zoom should use simple textured quads.");

    Require(
        renderer.GetGeometry()
                .GetBodyVertices()
                .size() ==
            6,
        "One simple ant should use one body quad.");

    renderer.UpdateGeometry(
        antStore.GetAnts(),
        viewport,
        1.0f);

    Require(
        renderer.GetMode() ==
            AntRenderingMode::Points,
        "Far zoom should use point rendering.");

    Require(
        renderer.GetPointVertices()
                .size() ==
            1,
        "Point rendering should contain only visible ants.");

    Require(
        renderer.GetStatistics()
                .vertexCount ==
            1,
        "One visible point ant should use one vertex.");

    renderer.SetMode(
        AntRenderingMode::SimpleQuads);

    renderer.UpdateGeometry(
        antStore.GetAnts(),
        viewport,
        1.0f);

    Require(
        renderer.GetMode() ==
            AntRenderingMode::
                SimpleQuads,
        "Manual mode should override automatic selection.");

    renderer.SetAutomaticMode(true);

    renderer.UpdateGeometry(
        antStore.GetAnts(),
        viewport,
        20.0f);

    Require(
        renderer.GetMode() ==
            AntRenderingMode::
                DetailedQuads,
        "Automatic selection should be restorable.");

    std::cout
        << "All ant renderer tests passed.\n";

    return 0;
}
