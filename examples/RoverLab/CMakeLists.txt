cmake_minimum_required(VERSION 3.25)
project(RoverLab LANGUAGES CXX)

if(NOT TARGET PipeFrame::Engine)
  if(DEFINED PIPEFRAME_ENGINE_LIBRARY AND DEFINED PIPEFRAME_SDK_ROOT)
    add_library(PipeFrame::Engine SHARED IMPORTED)

    set_target_properties(PipeFrame::Engine PROPERTIES
            IMPORTED_LOCATION "${PIPEFRAME_ENGINE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${PIPEFRAME_SDK_ROOT}/engine/include"
    )
  elseif(DEFINED PIPEFRAME_SDK_ROOT)
    add_subdirectory(
            "${PIPEFRAME_SDK_ROOT}/engine"
            "${CMAKE_BINARY_DIR}/PipeFrameEngine"
    )
  else()
    find_package(PipeFrame CONFIG REQUIRED)
  endif()
endif()

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
        Source/*.cpp
        Source/*.h
        Source/*.hpp
)

add_library(RoverLabRuntime SHARED ${PROJECT_SOURCES})

target_link_libraries(RoverLabRuntime PRIVATE PipeFrame::Engine)
target_include_directories(RoverLabRuntime PRIVATE Source)
target_compile_definitions(
        RoverLabRuntime PRIVATE PIPEFRAME_BUILDING_PROJECT_RUNTIME
)
target_compile_features(RoverLabRuntime PRIVATE cxx_std_20)

set_target_properties(RoverLabRuntime PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Build/$<CONFIG>"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Build/$<CONFIG>"
)

if(BUILD_TESTING)
  add_executable(RoverDriveTests Tests/RoverDriveTests.cpp)
  target_link_libraries(RoverDriveTests PRIVATE PipeFrame::Engine)
  target_include_directories(RoverDriveTests PRIVATE Source)
  target_compile_features(RoverDriveTests PRIVATE cxx_std_20)
  target_compile_options(RoverDriveTests PRIVATE
          $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-UNDEBUG>
  )
  add_test(NAME RoverDriveRegression COMMAND RoverDriveTests)

  add_executable(UltrasonicSensorTests Tests/UltrasonicSensorTests.cpp)
  target_link_libraries(UltrasonicSensorTests PRIVATE PipeFrame::Engine)
  target_include_directories(UltrasonicSensorTests PRIVATE Source)
  target_compile_features(UltrasonicSensorTests PRIVATE cxx_std_20)
  add_test(NAME UltrasonicSensorRegression COMMAND UltrasonicSensorTests)

  add_executable(RoverOdometryTests Tests/RoverOdometryTests.cpp)
  target_link_libraries(RoverOdometryTests PRIVATE PipeFrame::Engine)
  target_include_directories(RoverOdometryTests PRIVATE Source)
  target_compile_features(RoverOdometryTests PRIVATE cxx_std_20)
  add_test(NAME RoverOdometryRegression COMMAND RoverOdometryTests)

  add_executable(RoverSafetyTests Tests/RoverSafetyTests.cpp)
  target_link_libraries(RoverSafetyTests PRIVATE PipeFrame::Engine)
  target_include_directories(RoverSafetyTests PRIVATE Source)
  target_compile_features(RoverSafetyTests PRIVATE cxx_std_20)
  add_test(NAME RoverSafetyRegression COMMAND RoverSafetyTests)

  add_executable(RoverCorridorTests Tests/RoverCorridorTests.cpp)
  target_link_libraries(RoverCorridorTests PRIVATE PipeFrame::Engine)
  target_include_directories(RoverCorridorTests PRIVATE Source)
  target_compile_features(RoverCorridorTests PRIVATE cxx_std_20)
  add_test(NAME RoverCorridorRegression COMMAND RoverCorridorTests)

  add_executable(RoverRuntimeTests Tests/RoverRuntimeTests.cpp)
  target_link_libraries(RoverRuntimeTests PRIVATE PipeFrame::Engine)
  target_include_directories(RoverRuntimeTests PRIVATE Source)
  target_compile_features(RoverRuntimeTests PRIVATE cxx_std_20)
  add_test(NAME RoverRuntimeRegression COMMAND RoverRuntimeTests)

  add_executable(
    UltrasonicSchedulingTests
    Tests/UltrasonicSchedulingTests.cpp
  )

  target_link_libraries(
      UltrasonicSchedulingTests
      PRIVATE PipeFrame::Engine
  )

  target_include_directories(
      UltrasonicSchedulingTests
      PRIVATE Source
  )

  target_compile_features(
      UltrasonicSchedulingTests
      PRIVATE cxx_std_20
  )

  add_test(
      NAME UltrasonicSchedulingRegression
      COMMAND UltrasonicSchedulingTests
  )

  add_executable(
  UltrasonicNoiseTests
  Tests/UltrasonicNoiseTests.cpp
  )

  target_link_libraries(
    UltrasonicNoiseTests
    PRIVATE PipeFrame::Engine
  )

  target_include_directories(
    UltrasonicNoiseTests
    PRIVATE Source
  )

  target_compile_features(
    UltrasonicNoiseTests
    PRIVATE cxx_std_20
  )

  add_test(
    NAME UltrasonicNoiseRegression
    COMMAND UltrasonicNoiseTests
  )

  add_executable(
    RoverMappingTests
    Tests/RoverMappingTests.cpp
  )

  target_link_libraries(
    RoverMappingTests
    PRIVATE PipeFrame::Engine
  )

  target_include_directories(
    RoverMappingTests
    PRIVATE Source
  )

  target_compile_features(
    RoverMappingTests
    PRIVATE cxx_std_20
  )

  add_test(
    NAME RoverMappingRegression
    COMMAND RoverMappingTests
  )
  add_executable(RoverMapPanelTests Tests/RoverMapPanelTests.cpp)
  target_link_libraries(RoverMapPanelTests PRIVATE PipeFrame::Engine PipeFrame::BackendSFML)
  target_include_directories(RoverMapPanelTests PRIVATE Source)
  target_compile_features(RoverMapPanelTests PRIVATE cxx_std_20)
  target_compile_definitions(RoverMapPanelTests PRIVATE
    PIPEFRAME_TEST_FONT="${CMAKE_SOURCE_DIR}/assets/fonts/roboto_regular.ttf")
  add_test(NAME RoverMapPanelRegression COMMAND RoverMapPanelTests)
endif()

if(BUILD_TESTING)
  add_executable(LidarTests Tests/LidarTests.cpp)
  target_link_libraries(LidarTests PRIVATE PipeFrame::Engine)
  target_include_directories(LidarTests PRIVATE Source)
  target_compile_features(LidarTests PRIVATE cxx_std_20)
  add_test(NAME LidarRegression COMMAND LidarTests)
endif()

if(BUILD_TESTING)
  add_executable(RoverSensorSceneTests Tests/RoverSensorSceneTests.cpp
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneDocument.cpp
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneSerializer.cpp)
  target_link_libraries(RoverSensorSceneTests PRIVATE PipeFrame::Engine)
  target_include_directories(RoverSensorSceneTests PRIVATE Source
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor)
  target_compile_features(RoverSensorSceneTests PRIVATE cxx_std_20)
  target_compile_definitions(RoverSensorSceneTests PRIVATE
    ROVER_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME RoverSensorSceneRegression COMMAND RoverSensorSceneTests)
endif()

if(BUILD_TESTING)
  add_executable(RoverSolverTests Tests/RoverSolverTests.cpp)
  target_link_libraries(RoverSolverTests PRIVATE PipeFrame::Engine)
  target_include_directories(RoverSolverTests PRIVATE Source)
  target_compile_features(RoverSolverTests PRIVATE cxx_std_20)
  add_test(NAME RoverSolverRegression COMMAND RoverSolverTests)

  add_executable(RoverSolverSceneTests Tests/RoverSolverSceneTests.cpp
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneDocument.cpp
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneSerializer.cpp)
  target_link_libraries(RoverSolverSceneTests PRIVATE PipeFrame::Engine)
  target_include_directories(RoverSolverSceneTests PRIVATE Source
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor)
  target_compile_features(RoverSolverSceneTests PRIVATE cxx_std_20)
  target_compile_definitions(RoverSolverSceneTests PRIVATE
    ROVER_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME RoverSolverSceneRegression COMMAND RoverSolverSceneTests)
endif()

if(BUILD_TESTING)
  add_executable(RoverMissionTests Tests/RoverMissionTests.cpp
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneDocument.cpp
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneSerializer.cpp)
  target_link_libraries(RoverMissionTests PRIVATE PipeFrame::Engine PipeFrame::BackendSFML)
  target_include_directories(RoverMissionTests PRIVATE Source
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor)
  target_compile_definitions(RoverMissionTests PRIVATE
    ROVER_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_features(RoverMissionTests PRIVATE cxx_std_20)
  add_test(NAME RoverMissionRegression COMMAND RoverMissionTests)
endif()

if(BUILD_TESTING)
  add_executable(MicromouseTests Tests/MicromouseTests.cpp)
  target_link_libraries(MicromouseTests PRIVATE PipeFrame::Engine)
  target_include_directories(MicromouseTests PRIVATE Source)
  target_compile_features(MicromouseTests PRIVATE cxx_std_20)
  add_test(NAME MicromouseRegression COMMAND MicromouseTests)
  add_test(NAME RoverMicrobotRegression COMMAND RoverMissionTests Microbot.pfscene authored-limit maze-acceptance)
  set_tests_properties(RoverMicrobotRegression PROPERTIES TIMEOUT 600)
  foreach(level Medium Hard)
    add_test(NAME RoverMicrobot${level}Regression
      COMMAND RoverMissionTests Microbot${level}.pfscene authored-limit maze-acceptance)
    set_tests_properties(RoverMicrobot${level}Regression PROPERTIES TIMEOUT 600)
  endforeach()
endif()

# The native batch tool shares the editor's scene serializer, but does not open
# a Workbench or modify its live project. Build from the PipeFrame root checkout.
if(EXISTS "${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneSerializer.cpp")
  add_library(RoverBenchmarkSupport STATIC
    Tools/Benchmark/Benchmark.cpp
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneDocument.cpp
    ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneSerializer.cpp)
  target_link_libraries(RoverBenchmarkSupport PUBLIC PipeFrame::Engine)
  target_include_directories(RoverBenchmarkSupport PUBLIC Tools/Benchmark PRIVATE
    Source ${CMAKE_SOURCE_DIR}/apps/SimulationWorkbench/Editor)
  target_compile_features(RoverBenchmarkSupport PUBLIC cxx_std_20)

  add_executable(RoverLabBenchmark Tools/Benchmark/Main.cpp)
  target_link_libraries(RoverLabBenchmark PRIVATE RoverBenchmarkSupport)
  target_compile_definitions(RoverLabBenchmark PRIVATE
    ROVER_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

  if(BUILD_TESTING)
    add_executable(RoverBenchmarkTests Tests/RoverBenchmarkTests.cpp)
    target_link_libraries(RoverBenchmarkTests PRIVATE RoverBenchmarkSupport)
    target_compile_definitions(RoverBenchmarkTests PRIVATE
      ROVER_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME RoverBenchmarkRegression COMMAND RoverBenchmarkTests)
    set_tests_properties(RoverBenchmarkRegression PROPERTIES TIMEOUT 180)

    add_executable(RoverResultsTests Tests/RoverResultsTests.cpp)
    target_link_libraries(RoverResultsTests PRIVATE RoverBenchmarkSupport PipeFrame::BackendSFML)
    target_include_directories(RoverResultsTests PRIVATE Source)
    target_compile_definitions(RoverResultsTests PRIVATE
      ROVER_REPOSITORY_ROOT="${CMAKE_SOURCE_DIR}"
      PIPEFRAME_TEST_FONT="${CMAKE_SOURCE_DIR}/assets/fonts/roboto_regular.ttf")
    add_test(NAME RoverResultsRegression COMMAND RoverResultsTests)
  endif()
endif()
