#pragma once

#include <PipeFrame/Backend/SFML/SimulationDashboardHost.h>
#include <PipeFrame/UI/SimulationDashboard.h>
namespace pipeframe::backend::sfml {
// Native host/test integration only. Project code uses the declarative facade.
struct DashboardAccess {
    static NativeSimulationDashboard& Host(SimulationDashboard&);
};
}  // namespace pipeframe::backend::sfml
