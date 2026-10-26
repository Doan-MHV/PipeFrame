set(SAILBOAT_SIMULATION_SOURCES
        Source/Runtime/SailBoatSimulationRuntime.cpp
        Source/Editor/SailBoatDashboard.cpp
        Source/Configuration/SailBoatConfiguration.cpp
        Source/Rendering/SailBoatRenderer.cpp
        Source/Components/Boat.cpp
        Source/Systems/BoatMovementSystem.cpp
        Source/Components/PolarTable.cpp
        Source/Training/SailBoatAgent.cpp
        Source/Training/SailBoatPopulationTrainer.cpp
        Source/Training/SailBoatRaceTask.cpp
        Source/World/RaceSegment.cpp
        Source/World/RaceCourse.cpp
        Source/World/RaceCoursePersistence.cpp
)

add_library(SailBoatSimulationRuntime SHARED
        ${SAILBOAT_SIMULATION_SOURCES}
        Source/Plugin.cpp
)

target_compile_features(SailBoatSimulationRuntime
        PRIVATE
        cxx_std_20
)

target_compile_definitions(SailBoatSimulationRuntime
        PRIVATE
        PIPEFRAME_BUILDING_PROJECT_RUNTIME
)

target_include_directories(SailBoatSimulationRuntime
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(SailBoatSimulationRuntime
        PRIVATE
        PipeFrame::Engine
        PipeFrame::Audio
        PipeFrame::Learning
)

set_target_properties(SailBoatSimulationRuntime
        PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Build/$<CONFIG>"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Build/$<CONFIG>"
)

if (BUILD_TESTING)
    add_executable(SailBoatFoundationTests
            Tests/SailBoatFoundationTests.cpp
            ${SAILBOAT_SIMULATION_SOURCES}
    )

    target_compile_definitions(SailBoatFoundationTests PRIVATE PIPEFRAME_SAILBOAT_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

    target_compile_features(SailBoatFoundationTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(SailBoatFoundationTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(SailBoatFoundationTests
            PRIVATE
            PipeFrame::Engine
            PipeFrame::Audio
            PipeFrame::Learning
    )

    add_test(
            NAME SailBoatFoundationRegression
            COMMAND SailBoatFoundationTests
    )

    add_executable(SailBoatRaceEditorTests
            Tests/SailBoatRaceEditorTests.cpp
            Source/World/RaceCoursePersistence.cpp
    )

    target_compile_features(SailBoatRaceEditorTests PRIVATE cxx_std_20)
    target_include_directories(SailBoatRaceEditorTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    target_link_libraries(SailBoatRaceEditorTests PRIVATE PipeFrame::Engine)

    add_test(
            NAME SailBoatRaceEditorRegression
            COMMAND SailBoatRaceEditorTests
    )

    add_executable(SailBoatMovementTests
            Tests/SailBoatMovementTests.cpp
            Source/Components/Boat.cpp
            Source/Systems/BoatMovementSystem.cpp
            Source/Components/PolarTable.cpp
    )

    target_compile_features(SailBoatMovementTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(SailBoatMovementTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(SailBoatMovementTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME SailBoatMovementRegression
            COMMAND SailBoatMovementTests
    )

    add_executable(SailBoatRaceTaskTests
            Tests/SailBoatRaceTaskTests.cpp
            Source/Components/Boat.cpp
            Source/Systems/BoatMovementSystem.cpp
            Source/Components/PolarTable.cpp
            Source/Training/SailBoatRaceTask.cpp
            Source/World/RaceSegment.cpp
            Source/World/RaceCourse.cpp
    )

    target_compile_features(SailBoatRaceTaskTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(SailBoatRaceTaskTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(SailBoatRaceTaskTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME SailBoatRaceTaskRegression
            COMMAND SailBoatRaceTaskTests
    )

    add_executable(SailBoatNeatTests
            Tests/SailBoatNeatTests.cpp
    )

    target_compile_features(SailBoatNeatTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(SailBoatNeatTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(SailBoatNeatTests
            PRIVATE
            PipeFrame::Learning
    )

    add_test(
            NAME SailBoatNeatRegression
            COMMAND SailBoatNeatTests
    )

    add_executable(SailBoatPopulationTrainerTests
            Tests/SailBoatPopulationTrainerTests.cpp
            Source/Configuration/SailBoatConfiguration.cpp
            Source/Rendering/SailBoatRenderer.cpp
            Source/Components/Boat.cpp
            Source/Systems/BoatMovementSystem.cpp
            Source/Components/PolarTable.cpp
            Source/Training/SailBoatAgent.cpp
            Source/Training/SailBoatPopulationTrainer.cpp
            Source/Training/SailBoatRaceTask.cpp
            Source/World/RaceSegment.cpp
            Source/World/RaceCourse.cpp
    )

    target_compile_features(SailBoatPopulationTrainerTests
            PRIVATE
            cxx_std_20
    )

    target_compile_definitions(SailBoatPopulationTrainerTests
            PRIVATE
            PIPEFRAME_SAILBOAT_PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_include_directories(SailBoatPopulationTrainerTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(SailBoatPopulationTrainerTests
            PRIVATE
            PipeFrame::Engine
            PipeFrame::Learning
    )

    add_test(
            NAME SailBoatPopulationTrainerRegression
            COMMAND SailBoatPopulationTrainerTests
    )

    add_executable(SailBoatPerformanceTests
            Tests/SailBoatPerformanceTests.cpp
            Source/Configuration/SailBoatConfiguration.cpp
            Source/Components/Boat.cpp
            Source/Systems/BoatMovementSystem.cpp
            Source/Components/PolarTable.cpp
            Source/Training/SailBoatAgent.cpp
            Source/Training/SailBoatPopulationTrainer.cpp
            Source/Training/SailBoatRaceTask.cpp
            Source/World/RaceSegment.cpp
            Source/World/RaceCourse.cpp
    )

    target_compile_features(SailBoatPerformanceTests PRIVATE cxx_std_20)
    target_include_directories(SailBoatPerformanceTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    target_link_libraries(SailBoatPerformanceTests PRIVATE PipeFrame::Engine PipeFrame::Learning)

    add_test(
            NAME SailBoatPerformanceRegression
            COMMAND SailBoatPerformanceTests
    )

    add_executable(SailBoatProjectTests
            Tests/SailBoatProjectTests.cpp
            ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/ProjectManager.cpp
            ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/ProjectScaffolder.cpp
            ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneDocument.cpp
            ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneSerializer.cpp
    )

    target_compile_features(SailBoatProjectTests
            PRIVATE
            cxx_std_20
    )

    target_compile_definitions(SailBoatProjectTests
            PRIVATE
            PIPEFRAME_SAILBOAT_PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_include_directories(SailBoatProjectTests
            PRIVATE
            ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench
    )

    target_link_libraries(SailBoatProjectTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME SailBoatProjectRegression
            COMMAND SailBoatProjectTests
    )
endif ()

if(BUILD_TESTING)
    add_executable(SimulationDashboardTests Tests/SimulationDashboardTests.cpp)
    target_include_directories(SimulationDashboardTests PRIVATE Source ../AntSimulation/Source)
    target_link_libraries(SimulationDashboardTests PRIVATE SailBoatSimulationRuntime AntSimulationRuntime PipeFrame::Engine PipeFrame::Audio)
    target_compile_definitions(SimulationDashboardTests PRIVATE PIPEFRAME_EXAMPLES="${PROJECT_SOURCE_DIR}/examples")
    add_test(NAME SimulationDashboardAcceptance COMMAND SimulationDashboardTests)
endif()

if(BUILD_TESTING)
    add_executable(UIDashboardProfile Tests/UIDashboardProfile.cpp)
    target_include_directories(UIDashboardProfile PRIVATE Source ../AntSimulation/Source)
    target_link_libraries(UIDashboardProfile PRIVATE SailBoatSimulationRuntime AntSimulationRuntime PipeFrame::Engine PipeFrame::Audio)
    target_compile_definitions(UIDashboardProfile PRIVATE PIPEFRAME_PROFILE_EXAMPLES="${PROJECT_SOURCE_DIR}/examples")
    add_test(NAME UIDashboardStressProfile COMMAND UIDashboardProfile "${CMAKE_BINARY_DIR}/ui-dashboard-profile.csv")
    set_tests_properties(UIDashboardStressProfile PROPERTIES LABELS "performance;ui;stress" TIMEOUT 180)
endif()
