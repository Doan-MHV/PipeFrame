set(BASIC_SIMULATION_SOURCES
        Source/BasicSimulationRuntime.cpp
        Source/DemoAgent.cpp
        Source/Population.cpp
        Source/PopulationRenderer.cpp
        Source/PopulationSpatialGrid.cpp
)

add_library(BasicSimulationRuntime SHARED
        ${BASIC_SIMULATION_SOURCES}
        Source/Plugin.cpp
)

target_compile_features(BasicSimulationRuntime
        PRIVATE
        cxx_std_20
)

target_compile_definitions(BasicSimulationRuntime
        PRIVATE
        PIPEFRAME_BUILDING_PROJECT_RUNTIME
)

target_include_directories(BasicSimulationRuntime
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(BasicSimulationRuntime
        PRIVATE
        PipeFrame::Engine
)

set_target_properties(BasicSimulationRuntime
        PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/Build/$<CONFIG>"
        RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/Build/$<CONFIG>"
)

if (BUILD_TESTING)
    add_executable(BasicSimulationTests
            Tests/BasicSimulationTests.cpp
            ${BASIC_SIMULATION_SOURCES}
    )

    target_include_directories(BasicSimulationTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(BasicSimulationTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME BasicSimulationRegression
            COMMAND BasicSimulationTests
    )
endif()
# Native hosts/tests opt in explicitly; Engine no longer exports backend dependencies.
if(TARGET BasicSimulationRuntime)
    target_link_libraries(BasicSimulationRuntime PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET BasicSimulationTests)
    target_link_libraries(BasicSimulationTests PRIVATE PipeFrame::BackendSFML)
endif()
