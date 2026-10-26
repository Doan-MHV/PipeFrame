#pragma once
namespace pipeframe {
// Timestamp belongs to simulation time, not wall-clock time.
struct Event {
    double simulationTime{};
};
}
