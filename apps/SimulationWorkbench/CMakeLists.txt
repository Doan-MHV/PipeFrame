add_executable(SimulationWorkbench
        Main.cpp
        DiagnosticsOverlay.cpp
        DemoAgent.cpp
        WorkbenchScene.cpp
        Editor/SceneDocument.cpp
        Editor/SceneSerializer.cpp
        Editor/SimulationPanel.cpp
        Editor/HierarchyPanel.cpp
        Editor/ViewportToolbar.cpp
)

target_link_libraries(SimulationWorkbench
        PRIVATE
        PipeFrame::Engine
)

target_compile_definitions(SimulationWorkbench
        PRIVATE
        PIPEFRAME_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
)