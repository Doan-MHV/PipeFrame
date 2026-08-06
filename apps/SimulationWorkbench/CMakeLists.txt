add_executable(SimulationWorkbench
        Main.cpp
)

target_link_libraries(SimulationWorkbench
        PRIVATE
        PipeFrameEngine
)