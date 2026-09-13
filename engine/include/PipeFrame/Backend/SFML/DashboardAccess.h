#pragma once
#include <PipeFrame/UI/SimulationDashboard.h>
#include <PipeFrame/Backend/SFML/SimulationDashboardHost.h>
namespace pipeframe::backend::sfml {
// Native host/test integration only. Project code uses the declarative facade.
struct DashboardAccess { static NativeSimulationDashboard &Host(SimulationDashboard &); };
}
