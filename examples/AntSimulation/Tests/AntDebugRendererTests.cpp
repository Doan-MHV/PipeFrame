#include "World/Runtime/AntQuery.h"
#include "Configuration/AntConfiguration.h"
#include "World/Physics/AntBodySystem.h"
#include "World/Rendering/AntDebugRenderer.h"

#include <array>
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

    AntView selectedAnt =
        antStore.Create(
            1,
            AntRole::Follower,
            {10.0f, 10.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId selectedAntId =
        selectedAnt.GetId();

    selectedAnt.SetTarget({
        12.0f,
        10.0f,
    });

    AntView offscreenAnt =
        antStore.Create(
            1,
            AntRole::Follower,
            {100.0f, 100.0f},
            0.0f,
            0.0f,
            configuration);

    const AntId offscreenAntId =
        offscreenAnt.GetId();

    std::array<AntPhysicsBody, 2>
        bodies{};

    bodies[0].id = 1;
    bodies[0].antId =
        selectedAntId;
    bodies[0].position = {
        10.0f,
        10.0f,
    };
    bodies[0].moving = true;

    bodies[1].id = 2;
    bodies[1].antId =
        offscreenAntId;
    bodies[1].position = {
        100.0f,
        100.0f,
    };
    bodies[1].moving = false;

    const pipeframe::Rectanglef viewport{
        {0.0f, 0.0f},
        {20.0f, 20.0f},
    };

    AntDebugRenderer renderer;

    renderer.SetSelectedAnt(
        selectedAntId);

    renderer.SetTargetVisible(true);
    renderer.SetPhysicsDebugVisible(true);

    renderer.Update(
        antStore.GetAnts(),
        bodies,
        viewport);

    Require(
        renderer.GetSelectedAnt() ==
            selectedAntId,
        "Selected ant ID should be retained.");

    Require(
        renderer.GetSelectionVertices().size() ==
            AntDebugRenderer::
                CircleSegmentCount *
            6,
        "Selected ant should have a complete highlight ring.");

    Require(
        renderer.GetTargetVertices().size() ==
            AntDebugRenderer::
                CircleSegmentCount *
            3,
        "Selected ant target should have a filled circle.");

    Require(
        renderer.GetPhysicsCandidateCount() ==
            2,
        "Both physics bodies should be candidates.");

    Require(
        renderer.GetVisiblePhysicsBodyCount() ==
            1,
        "Only one physics body should be visible.");

    Require(
        renderer.GetPhysicsVertices().size() ==
            AntDebugRenderer::
                PhysicsCircleSegmentCount *
            3,
        "One physics body should have one debug circle.");

    Require(
        renderer.GetPhysicsVertices()[0].color ==
            AntDebugRenderer::
                MovingPhysicsColor,
        "Moving body should use the AntPezza moving-body color.");

    renderer.SetTargetVisible(false);

    Require(
        renderer.GetTargetVertices().empty(),
        "Disabling the target should clear target geometry.");

    renderer.SetPhysicsDebugVisible(false);

    Require(
        renderer.GetPhysicsVertices().empty(),
        "Disabling physics debug should clear physics geometry.");

    renderer.SetSelectedAnt(
        std::nullopt);

    Require(
        renderer.GetSelectionVertices().empty(),
        "Clearing selection should clear the selection ring.");

    std::cout
        << "All ant debug renderer tests passed.\n";

    return 0;
}