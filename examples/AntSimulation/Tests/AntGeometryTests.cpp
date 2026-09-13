#include "AntFixture.h"
#include "World/Runtime/AntView.h"
#include "Configuration/AntConfiguration.h"
#include "World/Rendering/AntGeometry.h"

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

    AntFixture ant(
        1,
        1,
        AntRole::Follower,
        {10.0f, 10.0f},
        0.0f,
        0.0f,
        configuration);

    ant.Identity().color = pipeframe::Color{
        20,
        40,
        60,
    };

    AntGeometry geometry;

    geometry.ResizeDetailed(1);

    Require(
        geometry.GetBodyVertices().size() ==
            18,
        "One detailed ant should use three body quads.");

    Require(
        geometry.GetLegVertices().size() ==
            36,
        "One detailed ant should use six leg quads.");

    Require(
        geometry.GetFoodVertices().size() ==
            6,
        "One ant should reserve one food quad.");

    geometry.UpdateDetailed(
        ant,
        0,
        configuration);

    Require(
        geometry.GetBodyVertices()[0].color ==
            ant.Identity().color,
        "Detailed body should use colony color.");

    Require(
        geometry.GetFoodVertices()[0]
                .color ==
            pipeframe::Color::Transparent,
        "An ant without food should have empty food geometry.");

    ant.SetState(
        ForagingState::ToHomeWithFood);

    geometry.UpdateDetailed(
        ant,
        0,
        configuration);

    Require(
        geometry.GetFoodVertices()[0]
                .color ==
            configuration.foodColor,
        "A carrying ant should render food.");

    configuration.dynamicAntColor = true;

    geometry.UpdateDetailed(
        ant,
        0,
        configuration);

    Require(
        geometry.GetBodyVertices()[0]
                .color ==
            configuration.toHomeAntColor,
        "Dynamic color should represent carrying state.");

    geometry.ClearDetailed(0);

    Require(
        geometry.GetBodyVertices()[0]
                .color ==
            pipeframe::Color::Transparent,
        "Clearing should hide detailed body geometry.");

    Require(
        geometry.GetLegVertices()[0]
                .color ==
            pipeframe::Color::Transparent,
        "Clearing should hide leg geometry.");

    geometry.ResizeSimple(1);

    Require(
        geometry.GetBodyVertices().size() ==
            6,
        "One simple ant should use one quad.");

    Require(
        geometry.GetLegVertices().empty(),
        "Simple geometry should not contain legs.");

    geometry.UpdateSimple(
        ant,
        0,
        configuration);

    Require(
        geometry.GetBodyVertices()[0]
                .color ==
            configuration.toHomeAntColor,
        "Simple geometry should use the same ant color rules.");

    geometry.ClearSimple(0);

    Require(
        geometry.GetBodyVertices()[0]
                .color ==
            pipeframe::Color::Transparent,
        "Simple geometry should support culling.");

    std::cout
        << "All ant geometry tests passed.\n";

    return 0;
}
