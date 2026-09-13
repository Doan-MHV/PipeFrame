cmake_minimum_required(VERSION 3.25)

# Runtime projects are loaded into the editor process. On macOS, both targets
# must therefore use the same dynamic SFML libraries; statically linking SFML
# into each binary duplicates its Objective-C Cocoa classes and crashes.
option(
        PIPEFRAME_USE_DYNAMIC_VCPKG_LIBRARIES
        "Use vcpkg's dynamic triplet for loadable PipeFrame runtime projects"
        ON
)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin"
        AND PIPEFRAME_USE_DYNAMIC_VCPKG_LIBRARIES)
    execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE PIPEFRAME_HOST_ARCHITECTURE
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (PIPEFRAME_HOST_ARCHITECTURE MATCHES "^(arm64|aarch64)$")
        set(PIPEFRAME_VCPKG_TRIPLET "arm64-osx-dynamic")
    else ()
        set(PIPEFRAME_VCPKG_TRIPLET "x64-osx-dynamic")
    endif ()

    set(
            VCPKG_TARGET_TRIPLET
            "${PIPEFRAME_VCPKG_TRIPLET}"
            CACHE STRING
            "PipeFrame vcpkg target triplet"
            FORCE
    )
endif ()

project(PipeFrame LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(
        PIPEFRAME_BUILD_EXAMPLES
        "Build PipeFrame example projects"
        ON
)

option(PIPEFRAME_ANT_REWORK_ONLY "Exclude SailBoat from engine/Ant rework builds and tests" OFF)
if(PIPEFRAME_ANT_REWORK_ONLY)
    add_compile_definitions(PIPEFRAME_ANT_REWORK_ONLY)
endif()

add_subdirectory(engine)
add_subdirectory(apps/SimulationWorkbench)
add_subdirectory(benchmarks/AntBenchmark)

if (PIPEFRAME_BUILD_EXAMPLES)
    add_subdirectory(examples/UIGallery)
    add_subdirectory(examples/ThermalLab)
    add_subdirectory(examples/BasicSimulation)
    add_subdirectory(examples/AntSimulation)
    if(NOT PIPEFRAME_ANT_REWORK_ONLY)
        add_subdirectory(examples/SailBoatSimulation)
    endif()

    # Development convenience for the shared CLion run
    # configurations. Both example runtime plugins will be
    # ready whenever the Workbench target is built.
    add_dependencies(
            SimulationWorkbench
            BasicSimulationRuntime
            AntSimulationRuntime
    )
    if(NOT PIPEFRAME_ANT_REWORK_ONLY)
        add_dependencies(SimulationWorkbench SailBoatSimulationRuntime)
        if(BUILD_TESTING)
            add_dependencies(WorkbenchUITests SailBoatSimulationRuntime)
        endif()
    endif()
    if (BUILD_TESTING)
        add_dependencies(WorkbenchUITests BasicSimulationRuntime AntSimulationRuntime)
    endif ()

    if (BUILD_TESTING)
        add_executable(ReferenceProjectConformanceTests
                engine/tests/ReferenceProjectConformanceTests.cpp)
        target_link_libraries(ReferenceProjectConformanceTests PRIVATE PipeFrame::Engine)
        target_compile_definitions(ReferenceProjectConformanceTests PRIVATE
                PIPEFRAME_BASIC_PROJECT="${CMAKE_SOURCE_DIR}/examples/BasicSimulation"
                PIPEFRAME_ANT_PROJECT="${CMAKE_SOURCE_DIR}/examples/AntSimulation"
                PIPEFRAME_SAILBOAT_PROJECT="${CMAKE_SOURCE_DIR}/examples/SailBoatSimulation"
                PIPEFRAME_BASIC_RUNTIME="$<TARGET_FILE:BasicSimulationRuntime>"
                PIPEFRAME_ANT_RUNTIME="$<TARGET_FILE:AntSimulationRuntime>")
        add_dependencies(ReferenceProjectConformanceTests BasicSimulationRuntime AntSimulationRuntime)
        if(NOT PIPEFRAME_ANT_REWORK_ONLY)
            target_compile_definitions(ReferenceProjectConformanceTests PRIVATE PIPEFRAME_SAILBOAT_RUNTIME="$<TARGET_FILE:SailBoatSimulationRuntime>")
            add_dependencies(ReferenceProjectConformanceTests SailBoatSimulationRuntime)
        endif()
        add_test(NAME ReferenceProjectConformance COMMAND ReferenceProjectConformanceTests)
    endif ()
endif ()
