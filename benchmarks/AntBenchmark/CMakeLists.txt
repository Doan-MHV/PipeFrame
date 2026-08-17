add_executable(AntBenchmark
        Main.cpp
        AntPopulation.cpp
        AntBatchRenderer.cpp
        AntBenchmarkScene.cpp
        AntSpatialGrid.cpp
        AntBenchmarkOverlay.cpp
        AntSeparationSystem.cpp
)

target_link_libraries(AntBenchmark
        PRIVATE
        PipeFrame::Engine
)

target_compile_definitions(AntBenchmark
        PRIVATE
        PIPEFRAME_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
)