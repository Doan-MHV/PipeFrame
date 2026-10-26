#pragma once

#include <PipeFrame/UI/ProjectUIHost.h>
#include <PipeFrame/UI/ViewPanel.h>

#include <cmath>
#include <iomanip>
#include <numbers>
#include <sstream>
#include <string>

#include "Components/RoverDriveComponent.h"
#include "Events/RoverSafetyEvent.h"
#include "World/ProjectWorld.h"

namespace RoverLab {

class RoverSpeedPanel final : public pipeframe::ui::ViewPanel {
public:
    RoverSpeedPanel(ProjectWorld& world, pipeframe::ui::ProjectUIHost&) : world(world) {}

    void OnUpdate(float) override {
        std::ostringstream next;
        next << std::fixed << std::setprecision(3);

        int count = 0;

        auto& components = world.simulation.GetScene().Components();

        auto appendRoverReadout = [&](pipeframe::ecs::Entity entity) {
            const auto& body = *components.Get<pipeframe::KinematicBody2DComponent>(entity);

            if (!world.simulation.GetScene().IsActive(entity)) {
                return;
            }

            next << "Rover " << ++count << '\n';

            const auto solverStatus = world.simulation.solver.Status(entity);
            const bool solverActive = solverStatus != "Solver inactive";
            next << "Controller: "
                 << (solverActive ? solverStatus : world.simulation.navigation.Status(entity))
                 << '\n';

            next << "Safety: " << SafetyStatusText(world.simulation.drive.SafetyStatus(entity))
                 << '\n';

            const bool stopped = std::hypot(body.velocity.x, body.velocity.y) < 1e-5 &&
                                 std::abs(body.angularVelocity) < 1e-5;
            if (stopped) {
                const auto status = world.simulation.drive.SafetyStatus(entity);
                auto reason = world.simulation.drive.StopReason(entity);
                if (status == RoverSafetyStatus::Stopped) {
                    const auto navigationReason = solverActive
                                                      ? solverStatus
                                                      : world.simulation.navigation.StopReason(entity);
                    if (!navigationReason.empty()) reason = navigationReason;
                } else if (status == RoverSafetyStatus::Clear) {
                    reason = "Physics constrained motion; navigation is checking recovery";
                }
                next << "Stop reason: " << reason << '\n';
            }

            next << "Body speed: " << std::hypot(body.velocity.x, body.velocity.y) << " m/s\n";

            if (const auto* hardware = components.Get<MicromouseHardwareComponent>(entity)) {
                next << "Hardware: " << hardware->mass * 1000 << " g | "
                     << hardware->length * 1000 << " x " << hardware->width * 1000 << " mm\n";
                next << "Wheel limit: " << hardware->WheelSpeed() << " m/s | Accel: "
                     << hardware->Acceleration() << " m/s2\n";
            }

            const auto* estimate = world.simulation.odometry.Find(entity);

            if (!estimate) {
                next << "Odometry: waiting for Play\n\n";
                return;
            }

            next << "Estimated X: " << estimate->x << " m\n";

            next << "Estimated Y: " << estimate->y << " m\n";

            next << "Heading: " << estimate->heading * 180.0 / std::numbers::pi << " deg\n";

            next << "Distance: " << estimate->distance << " m\n\n";
        };

        components.Each<RoverDriveComponent, pipeframe::KinematicBody2DComponent>(
            appendRoverReadout);

        const std::string value = count ? next.str() : "No rover in this scene";

        if (value != readout) {
            readout = value;
            RefreshView();
        }
    }

protected:
    pipeframe::ui::View BuildView() override {
        using namespace pipeframe::ui;

        return views::Scroll("motion-scroll",
            views::Column("motion",
                {
                    views::Text("title", "ROVER MOTION"),
                    views::Text("value", readout).FitHeight(),
                    views::Button("hide", "HIDE", [this] { Hide(); }),
                })
                .Padding(12)
                .Spacing(8)
                .FitHeight())
            .FillHeight();
    }

private:
    ProjectWorld& world;
    std::string readout = "Waiting for rover";
};

}  // namespace RoverLab
