add_executable(SimulationWorkbench
        Main.cpp
        EditorCameraController.cpp
        DiagnosticsOverlay.cpp
)

target_link_libraries(SimulationWorkbench
        PRIVATE
        PipeFrame::Engine
)

target_compile_definitions(SimulationWorkbench
        PRIVATE
        PIPEFRAME_WORKBENCH_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)