#include <PipeFrame/UI/SimulationDashboard.h>
#include <PipeFrame/UI/ViewPanel.h>
#include <PipeFrame/Project/SceneProjectRuntime.h>
#if __has_include(<SFML/Graphics.hpp>)
#error PipeFrame::Engine must not export native backend include directories to project consumers
#endif
// This target links Engine (unlike the standalone header probes), checking its
// actual CMake usage requirements from a normal project consumer's perspective.
