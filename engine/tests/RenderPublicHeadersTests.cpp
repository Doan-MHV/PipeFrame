#include <PipeFrame/UI/ViewPanel.h>
#include <PipeFrame/UI/SimulationDashboard.h>
#include <PipeFrame/Render/CameraController2D.h>
#include <PipeFrame/Render/Camera2D.h>
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Project/ProjectRuntime.h>
int main() { Camera2D camera; camera.SetCenter({1,2}); return camera.GetCenter().x!=1; }
