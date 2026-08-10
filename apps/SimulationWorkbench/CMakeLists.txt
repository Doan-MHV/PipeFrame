add_executable(SimulationWorkbench
        Main.cpp
        EditorCameraController.cpp
)

target_link_libraries(SimulationWorkbench
        PRIVATE
        PipeFrame::Engine
)