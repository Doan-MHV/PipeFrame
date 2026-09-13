#include "AntFixture.h"
#include "World/Runtime/AntView.h"
#include "Configuration/AntConfiguration.h"
#include "World/Rendering/AntGeometry.h"
#include "World/Rendering/ShadowRenderer.h"

#include <cmath>
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

bool NearlyEqual(
    const float first,
    const float second,
    const float tolerance = 0.0001f
) {
    return std::abs(first - second) <=
           tolerance;
}

} // namespace

int main() {
    using namespace ant_simulation;

    AntConfiguration configuration;

    AntFixture ant(
        1,
        1,
        AntRole::Follower,
        {10.0f, 10.0f},
        0.0f,
        0.0f,
        configuration);

    ant.SetState(
        ForagingState::ToHomeWithFood);

    AntGeometry geometry;
    geometry.ResizeDetailed(1);

    geometry.UpdateDetailed(
        ant,
        0,
        configuration);

    ShadowRenderer shadowRenderer;

    shadowRenderer.Update(
        geometry,
        AntRenderingMode::
            DetailedQuads);

    Require(
        shadowRenderer
                .GetBodyVertices()
                .size() ==
            geometry
                .GetBodyVertices()
                .size(),
        "Detailed shadows should copy body geometry.");

    Require(
        shadowRenderer
                .GetLegVertices()
                .size() ==
            geometry
                .GetLegVertices()
                .size(),
        "Detailed shadows should copy leg geometry.");

    Require(
        shadowRenderer
                .GetFoodVertices()
                .size() ==
            geometry
                .GetFoodVertices()
                .size(),
        "Detailed shadows should copy carried-food geometry.");

    const pipeframe::Vertex2D &sourceVertex =
        geometry.GetBodyVertices()[0];

    const pipeframe::Vertex2D &shadowVertex =
        shadowRenderer
            .GetBodyVertices()[0];

    Require(
        NearlyEqual(
            shadowVertex.position.x,
            sourceVertex.position.x +
                ShadowRenderer::
                    DefaultOffset.x),
        "Shadow X should include configured offset.");

    Require(
        NearlyEqual(
            shadowVertex.position.y,
            sourceVertex.position.y +
                ShadowRenderer::
                    DefaultOffset.y),
        "Shadow Y should include configured offset.");

    Require(
        shadowVertex.color ==
            ShadowRenderer::DefaultColor,
        "Shadow geometry should use shadow color.");

    Require(
        ShadowRenderer::DefaultColor.r <= 16 &&
            ShadowRenderer::DefaultColor.g <= 16 &&
            ShadowRenderer::DefaultColor.b <= 16 &&
            ShadowRenderer::DefaultColor.a >= 64 &&
            ShadowRenderer::DefaultColor.a <= 160,
        "Ant shadows should remain dark and translucent.");

    geometry.ResizeSimple(1);

    geometry.UpdateSimple(
        ant,
        0,
        configuration);

    shadowRenderer.Update(
        geometry,
        AntRenderingMode::
            SimpleQuads);

    Require(
        shadowRenderer
                .GetBodyVertices()
                .size() ==
            6,
        "Simple mode should contain one shadow body quad.");

    Require(
        shadowRenderer
                .GetLegVertices()
                .empty(),
        "Simple shadows should not contain legs.");

    shadowRenderer.Update(
        geometry,
        AntRenderingMode::Points);

    Require(
        shadowRenderer
                .GetBodyVertices()
                .empty(),
        "Point mode should skip shadow geometry.");

    shadowRenderer.SetEnabled(false);

    shadowRenderer.Update(
        geometry,
        AntRenderingMode::
            SimpleQuads);

    Require(
        shadowRenderer
                .GetBodyVertices()
                .empty(),
        "Disabled shadows should remain empty.");

    std::cout
        << "All shadow renderer tests passed.\n";

    return 0;
}
