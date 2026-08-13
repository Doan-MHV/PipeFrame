add_executable(AntBenchmark
        Main.cpp
        AntPopulation.cpp
        AntBatchRenderer.cpp
        AntBenchmarkScene.cpp
)

target_link_libraries(AntBenchmark
        PRIVATE
        PipeFrame::Engine
)