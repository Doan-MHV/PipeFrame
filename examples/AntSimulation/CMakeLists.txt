# Discover the first generated registration header as well as subsequent edits.
# A previously-false __has_include is not itself a compiler depfile dependency.
file(GLOB ANT_GENERATED_REGISTRATION CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/Runtime/GeneratedRegistration.h")
if (ANT_GENERATED_REGISTRATION)
    set_property(SOURCE Source/Runtime/AntSimulationRuntime.cpp APPEND PROPERTY
            OBJECT_DEPENDS "${ANT_GENERATED_REGISTRATION}")
endif ()

set(ANT_SIMULATION_SOURCES
        Source/World/AntWorld.cpp
        Source/Runtime/AntSimulationRuntime.cpp
        Source/World/Runtime/AntWorldAuthoring.cpp
        Source/World/Rendering/AntRenderingWorld.cpp
        Source/Editor/AntDashboard.cpp
        Source/Editor/AntUIHost.cpp
        Source/Configuration/AntConfiguration.cpp
        Source/World/Runtime/Environment/Marker.cpp
        Source/World/Runtime/Environment/AntEnvironment.cpp
        Source/World/Runtime/Environment/WallBuilder.cpp
        Source/World/Runtime/Environment/WorldMapLoader.cpp
        Source/Runtime/AntRegistration.cpp

        Source/World/Physics/AntLegPose.cpp
        Source/World/Runtime/AntView.cpp
        Source/World/Runtime/Environment/GridRaycast.cpp
        Source/World/Runtime/Systems/MarkerSampler.cpp
        Source/World/Runtime/AntQuery.cpp
        Source/World/Runtime/ColonyView.cpp
        Source/World/Runtime/Systems/ColonyLifecycleSystem.cpp
        Source/World/Runtime/Systems/WorkerBehavior.cpp
        Source/World/Physics/CollisionGrid.cpp
        Source/World/Physics/AntContactSystem.cpp
        Source/World/Physics/AntBodySystem.cpp
        Source/World/Physics/ContactSolver.cpp
        Source/World/Physics/AntMovementSystem.cpp
        Source/World/Runtime/Systems/AntForagingSystem.cpp
        Source/World/Runtime/Systems/AntCleanupSystem.cpp
        Source/World/Physics/AntAvoidanceSystem.cpp
        Source/World/Rendering/AntGeometry.cpp
        Source/World/Rendering/AntRenderer.cpp
        Source/World/Rendering/ShadowRenderer.cpp
        Source/World/Rendering/MarkerRenderer.cpp
        Source/World/Rendering/WallRenderer.cpp
        Source/World/Rendering/EnvironmentRenderer.cpp
        Source/World/Rendering/AntDebugRenderer.cpp
        Source/Editor/AntEditorTool.cpp
        Source/Editor/AntEditorToolRenderer.cpp
        Source/Editor/AntInspector.cpp
        Source/Editor/ColonyInspector.cpp
        Source/World/Runtime/ColonyHistory.cpp
)

add_library(AntSimulationRuntime SHARED
        ${ANT_SIMULATION_SOURCES}
        Source/Plugin.cpp
)

# Header-only world modules and entity/component recipes need explicit target
# membership so IDEs give them this target's include paths and C++ standard.
# CONFIGURE_DEPENDS also discovers headers created by the authoring tools.
file(GLOB_RECURSE ANT_SIMULATION_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp")
target_sources(AntSimulationRuntime PRIVATE ${ANT_SIMULATION_HEADERS})

target_compile_features(AntSimulationRuntime
        PRIVATE
        cxx_std_20
)

target_compile_definitions(AntSimulationRuntime
        PRIVATE
        PIPEFRAME_BUILDING_PROJECT_RUNTIME
)

target_include_directories(AntSimulationRuntime
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(AntSimulationRuntime
        PRIVATE
        PipeFrame::Engine
)

set_target_properties(AntSimulationRuntime
        PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/Build/$<CONFIG>"
        RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/Build/$<CONFIG>"
)

if (BUILD_TESTING)
    add_executable(AntEnvironmentAuthoringTests Tests/AntEnvironmentAuthoringTests.cpp ${ANT_SIMULATION_SOURCES}
            ${PROJECT_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneDocument.cpp
            ${PROJECT_SOURCE_DIR}/apps/SimulationWorkbench/Editor/SceneSerializer.cpp)
    target_compile_features(AntEnvironmentAuthoringTests PRIVATE cxx_std_20)
    target_include_directories(AntEnvironmentAuthoringTests PRIVATE Source ${PROJECT_SOURCE_DIR}/apps/SimulationWorkbench/Editor)
    target_link_libraries(AntEnvironmentAuthoringTests PRIVATE PipeFrame::Engine)
    add_test(NAME AntEnvironmentAuthoringAcceptance COMMAND AntEnvironmentAuthoringTests)

    add_executable(AntSimulationTests
            Tests/AntSimulationTests.cpp
            ${ANT_SIMULATION_SOURCES}
    )

    target_compile_features(AntSimulationTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntSimulationTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntSimulationTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntSimulationRegression
            COMMAND AntSimulationTests
    )

    add_executable(AntConfigurationTests
            Tests/AntConfigurationTests.cpp
            Source/Configuration/AntConfiguration.cpp
    )

    target_compile_features(AntConfigurationTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntConfigurationTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntConfigurationTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntConfigurationRegression
            COMMAND AntConfigurationTests
    )

    add_executable(AntMarkerTests
            Tests/MarkerTests.cpp
            Source/World/Runtime/Environment/Marker.cpp
    )

    target_compile_features(AntMarkerTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntMarkerTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntMarkerTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntMarkerRegression
            COMMAND AntMarkerTests
    )

    add_executable(AntEnvironmentTests
            Tests/EnvironmentTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
    )

    target_compile_features(AntEnvironmentTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntEnvironmentTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntEnvironmentTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntEnvironmentRegression
            COMMAND AntEnvironmentTests
    )

    add_executable(AntFoodTests
            Tests/FoodTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
    )

    target_compile_features(AntFoodTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntFoodTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntFoodTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntFoodRegression
            COMMAND AntFoodTests
    )

    add_executable(AntWallBuilderTests
            Tests/WallBuilderTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
    )

    target_compile_features(AntWallBuilderTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntWallBuilderTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntWallBuilderTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntWallBuilderRegression
            COMMAND AntWallBuilderTests
    )

    add_executable(AntWorldMapLoaderTests
            Tests/WorldMapLoaderTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/World/Runtime/Environment/WorldMapLoader.cpp
    )

    target_compile_features(AntWorldMapLoaderTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntWorldMapLoaderTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntWorldMapLoaderTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntWorldMapLoaderRegression
            COMMAND AntWorldMapLoaderTests
    )

    add_executable(AntBehaviorTests
            Tests/AntBehaviorTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
    )

    target_compile_features(AntBehaviorTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntBehaviorTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntBehaviorTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntBehaviorRegression
            COMMAND AntBehaviorTests
    )

    add_executable(AntMarkerSamplerTests
            Tests/MarkerSamplerTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/World/Runtime/Environment/GridRaycast.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/Systems/MarkerSampler.cpp
    )

    target_compile_features(AntMarkerSamplerTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntMarkerSamplerTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntMarkerSamplerTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntMarkerSamplerRegression
            COMMAND AntMarkerSamplerTests
    )

    add_executable(AntColonyTests
            Tests/ColonyTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Runtime/ColonyView.cpp
            Source/World/Runtime/Systems/ColonyLifecycleSystem.cpp
    )

    target_compile_features(AntColonyTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntColonyTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntColonyTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntColonyRegression
            COMMAND AntColonyTests
    )

    add_executable(AntWorkerBehaviorTests
            Tests/WorkerBehaviorTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/GridRaycast.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/Systems/MarkerSampler.cpp
            Source/World/Runtime/Systems/WorkerBehavior.cpp
            Source/World/Runtime/ColonyView.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
    )

    target_compile_features(AntWorkerBehaviorTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntWorkerBehaviorTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntWorkerBehaviorTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntWorkerBehaviorRegression
            COMMAND AntWorkerBehaviorTests
    )

    add_executable(AntContactSystemTests
            Tests/AntContactSystemTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Physics/CollisionGrid.cpp
            Source/World/Physics/AntContactSystem.cpp
    )

    target_compile_features(AntContactSystemTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntContactSystemTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntContactSystemTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntContactSystemRegression
            COMMAND AntContactSystemTests
    )

    add_executable(AntPhysicsWorldTests
            Tests/AntPhysicsWorldTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Physics/AntBodySystem.cpp
    )

    target_compile_features(AntPhysicsWorldTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntPhysicsWorldTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntPhysicsWorldTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntPhysicsWorldRegression
            COMMAND AntPhysicsWorldTests
    )

    add_executable(AntContactSolverTests
            Tests/ContactSolverTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Physics/CollisionGrid.cpp
            Source/World/Physics/AntBodySystem.cpp
            Source/World/Physics/ContactSolver.cpp
    )

    target_compile_features(AntContactSolverTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntContactSolverTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntContactSolverTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntContactSolverRegression
            COMMAND AntContactSolverTests
    )

    add_executable(AntSystemIntegrationTests
            Tests/AntSystemIntegrationTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/World/Runtime/Environment/GridRaycast.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Runtime/Systems/MarkerSampler.cpp
            Source/World/Runtime/Systems/WorkerBehavior.cpp
            Source/World/Physics/AntContactSystem.cpp
            Source/World/Physics/AntMovementSystem.cpp
            Source/World/Runtime/Systems/AntForagingSystem.cpp
            Source/World/Runtime/Systems/AntCleanupSystem.cpp
            Source/World/Runtime/ColonyView.cpp
            Source/World/Runtime/Systems/ColonyLifecycleSystem.cpp
            Source/World/Physics/CollisionGrid.cpp
            Source/World/Physics/AntBodySystem.cpp
            Source/World/Physics/ContactSolver.cpp
            Source/World/Physics/AntAvoidanceSystem.cpp
    )

    target_compile_features(AntSystemIntegrationTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntSystemIntegrationTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntSystemIntegrationTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntSimulationPipelineRegression
            COMMAND AntSystemIntegrationTests
    )

    add_executable(AntAvoidanceSystemTests
            Tests/AntAvoidanceSystemTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Physics/CollisionGrid.cpp
            Source/World/Physics/AntBodySystem.cpp
            Source/World/Physics/AntAvoidanceSystem.cpp
    )

    target_compile_features(AntAvoidanceSystemTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntAvoidanceSystemTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntAvoidanceSystemTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntAvoidanceSystemRegression
            COMMAND AntAvoidanceSystemTests
    )

    add_executable(AntGeometryTests
            Tests/AntGeometryTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Rendering/AntGeometry.cpp
    )

    target_compile_features(AntGeometryTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntGeometryTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntGeometryTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntGeometryRegression
            COMMAND AntGeometryTests
    )

    add_executable(AntRendererTests
            Tests/AntRendererTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Rendering/AntGeometry.cpp
            Source/World/Rendering/AntRenderer.cpp
    )

    target_compile_features(AntRendererTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntRendererTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntRendererTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntRendererRegression
            COMMAND AntRendererTests
    )

    add_executable(AntShadowRendererTests
            Tests/ShadowRendererTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Rendering/AntGeometry.cpp
            Source/World/Rendering/AntRenderer.cpp
            Source/World/Rendering/ShadowRenderer.cpp
    )

    target_compile_features(AntShadowRendererTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntShadowRendererTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntShadowRendererTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntShadowRendererRegression
            COMMAND AntShadowRendererTests
    )

    add_executable(AntMarkerRendererTests
            Tests/MarkerRendererTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/World/Rendering/MarkerRenderer.cpp
    )

    target_compile_features(AntMarkerRendererTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntMarkerRendererTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntMarkerRendererTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntMarkerRendererRegression
            COMMAND AntMarkerRendererTests
    )

    add_executable(AntWallRendererTests
            Tests/WallRendererTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/World/Rendering/WallRenderer.cpp
    )

    target_compile_features(AntWallRendererTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntWallRendererTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntWallRendererTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntWallRendererRegression
            COMMAND AntWallRendererTests
    )

    add_executable(AntEnvironmentRendererTests
            Tests/EnvironmentRendererTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/World/Runtime/ColonyView.cpp
            Source/World/Rendering/MarkerRenderer.cpp
            Source/World/Rendering/WallRenderer.cpp
            Source/World/Rendering/EnvironmentRenderer.cpp
    )

    target_compile_features(AntEnvironmentRendererTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntEnvironmentRendererTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntEnvironmentRendererTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntEnvironmentRendererRegression
            COMMAND AntEnvironmentRendererTests
    )

    add_executable(AntDebugRendererTests
            Tests/AntDebugRendererTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Rendering/AntDebugRenderer.cpp
    )

    target_compile_features(AntDebugRendererTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntDebugRendererTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntDebugRendererTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntDebugRendererRegression
            COMMAND AntDebugRendererTests
    )

    add_executable(AntEditorToolTests
            Tests/AntEditorToolTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/Editor/AntEditorTool.cpp
    )

    target_compile_features(AntEditorToolTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntEditorToolTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntEditorToolTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntEditorToolRegression
            COMMAND AntEditorToolTests
    )

    add_executable(AntEditorToolRendererTests
            Tests/AntEditorToolRendererTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/Editor/AntEditorTool.cpp
            Source/Editor/AntEditorToolRenderer.cpp
    )

    target_compile_features(AntEditorToolRendererTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntEditorToolRendererTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntEditorToolRendererTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntEditorToolRendererRegression
            COMMAND AntEditorToolRendererTests
    )

    add_executable(AntInspectorTests
            Tests/AntInspectorTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/Editor/AntInspector.cpp
    )

    target_compile_features(AntInspectorTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntInspectorTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntInspectorTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntInspectorRegression
            COMMAND AntInspectorTests
    )

    add_executable(AntColonyInspectorTests
            Tests/ColonyInspectorTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/ColonyView.cpp
            Source/Editor/ColonyInspector.cpp
    )

    target_compile_features(AntColonyInspectorTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntColonyInspectorTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntColonyInspectorTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntColonyInspectorRegression
            COMMAND AntColonyInspectorTests
    )

    add_executable(AntSimulationWorldTests
            Tests/AntSimulationWorldTests.cpp
            Source/World/AntWorld.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/World/Runtime/Environment/GridRaycast.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Runtime/Systems/MarkerSampler.cpp
            Source/World/Runtime/Systems/WorkerBehavior.cpp
            Source/World/Physics/AntContactSystem.cpp
            Source/World/Physics/AntAvoidanceSystem.cpp
            Source/World/Physics/AntMovementSystem.cpp
            Source/World/Runtime/Systems/AntForagingSystem.cpp
            Source/World/Runtime/Systems/AntCleanupSystem.cpp
            Source/World/Runtime/ColonyView.cpp
            Source/World/Runtime/Systems/ColonyLifecycleSystem.cpp
            Source/World/Physics/CollisionGrid.cpp
            Source/World/Physics/AntBodySystem.cpp
            Source/World/Physics/ContactSolver.cpp
    )

    target_compile_features(AntSimulationWorldTests
            PRIVATE
            cxx_std_20
    )

    target_include_directories(AntSimulationWorldTests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(AntSimulationWorldTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME AntSimulationWorldRegression
            COMMAND AntSimulationWorldTests
    )

    add_executable(AntBehaviorParityTests
            Tests/AntBehaviorParityTests.cpp
            Source/World/AntWorld.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/World/Runtime/Environment/GridRaycast.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Runtime/Systems/MarkerSampler.cpp
            Source/World/Runtime/Systems/WorkerBehavior.cpp
            Source/World/Physics/AntContactSystem.cpp
            Source/World/Physics/AntAvoidanceSystem.cpp
            Source/World/Physics/AntMovementSystem.cpp
            Source/World/Runtime/Systems/AntForagingSystem.cpp
            Source/World/Runtime/Systems/AntCleanupSystem.cpp
            Source/World/Runtime/ColonyView.cpp
            Source/World/Runtime/Systems/ColonyLifecycleSystem.cpp
            Source/World/Physics/CollisionGrid.cpp
            Source/World/Physics/AntBodySystem.cpp
            Source/World/Physics/ContactSolver.cpp
    )

    target_compile_features(AntBehaviorParityTests PRIVATE cxx_std_20)
    target_include_directories(AntBehaviorParityTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    target_link_libraries(AntBehaviorParityTests PRIVATE PipeFrame::Engine)

    add_test(NAME AntBehaviorParityRegression COMMAND AntBehaviorParityTests)

    add_executable(AntStressTests
            Tests/AntStressTests.cpp
            Source/World/AntWorld.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp
            Source/World/Runtime/Environment/GridRaycast.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Runtime/Systems/MarkerSampler.cpp
            Source/World/Runtime/Systems/WorkerBehavior.cpp
            Source/World/Physics/AntContactSystem.cpp
            Source/World/Physics/AntAvoidanceSystem.cpp
            Source/World/Physics/AntMovementSystem.cpp
            Source/World/Runtime/Systems/AntForagingSystem.cpp
            Source/World/Runtime/Systems/AntCleanupSystem.cpp
            Source/World/Runtime/ColonyView.cpp
            Source/World/Runtime/Systems/ColonyLifecycleSystem.cpp
            Source/World/Physics/CollisionGrid.cpp
            Source/World/Physics/AntBodySystem.cpp
            Source/World/Physics/ContactSolver.cpp
    )

    target_compile_features(AntStressTests PRIVATE cxx_std_20)
    target_include_directories(AntStressTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    target_link_libraries(AntStressTests PRIVATE PipeFrame::Engine)

    add_test(NAME AntStressRegression COMMAND AntStressTests)
    set_tests_properties(AntStressRegression PROPERTIES
            LABELS "stress;performance"
            TIMEOUT 180
    )

    add_executable(AntRenderPerformanceTests
            Tests/AntRenderPerformanceTests.cpp
            Source/Configuration/AntConfiguration.cpp
            Source/World/Runtime/Environment/Marker.cpp
            Source/World/Runtime/Environment/AntEnvironment.cpp
            Source/World/Runtime/Environment/WallBuilder.cpp

            Source/World/Physics/AntLegPose.cpp
            Source/World/Runtime/AntView.cpp
            Source/World/Runtime/AntQuery.cpp
            Source/World/Runtime/ColonyView.cpp
            Source/World/Rendering/AntGeometry.cpp
            Source/World/Rendering/AntRenderer.cpp
            Source/World/Rendering/ShadowRenderer.cpp
            Source/World/Rendering/MarkerRenderer.cpp
            Source/World/Rendering/WallRenderer.cpp
            Source/World/Rendering/EnvironmentRenderer.cpp
    )

    target_compile_features(AntRenderPerformanceTests PRIVATE cxx_std_20)
    target_compile_definitions(AntRenderPerformanceTests PRIVATE
            ANT_SIMULATION_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
    )
    target_include_directories(AntRenderPerformanceTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    target_link_libraries(AntRenderPerformanceTests PRIVATE PipeFrame::Engine)

    add_test(NAME AntRenderPerformanceRegression COMMAND AntRenderPerformanceTests)
    set_tests_properties(AntRenderPerformanceRegression PROPERTIES
            LABELS "stress;performance;rendering"
            TIMEOUT 180
    )

    add_executable(
            AntColonyHistoryTests

            Tests/ColonyHistoryTests.cpp

            Source/Configuration/AntConfiguration.cpp

            Source/World/Runtime/Environment/Marker.cpp

            Source/World/Runtime/ColonyView.cpp
            Source/World/Runtime/ColonyHistory.cpp
    )

    target_include_directories(
            AntColonyHistoryTests

            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(
            AntColonyHistoryTests

            PRIVATE
            PipeFrameEngine
    )

    add_test(
            NAME AntColonyHistoryTests
            COMMAND AntColonyHistoryTests
    )

endif ()

if (BUILD_TESTING)
    add_executable(AntDashboardTests Tests/AntDashboardTests.cpp)
    target_include_directories(AntDashboardTests PRIVATE Source)
    target_link_libraries(AntDashboardTests PRIVATE AntSimulationRuntime PipeFrame::Engine)
    target_compile_definitions(AntDashboardTests PRIVATE PIPEFRAME_ANT_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME AntDashboardAcceptance COMMAND AntDashboardTests "${CMAKE_BINARY_DIR}/ant-rework-baseline.png")
endif ()

# Compile actual author-facing UI code without Engine's native include directories.
if (BUILD_TESTING)
    add_library(AntUIBoundaryCompile OBJECT
            Source/Editor/AntUIHost.cpp Source/Editor/AntDashboard.cpp)
    target_include_directories(AntUIBoundaryCompile PRIVATE
            "${PROJECT_SOURCE_DIR}/engine/include" "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(AntUIBoundaryCompile PRIVATE cxx_std_20)
endif ()

# Native hosts/tests opt in explicitly; Engine no longer exports backend dependencies.
if (TARGET AntRenderPerformanceTests)
    target_link_libraries(AntRenderPerformanceTests PRIVATE PipeFrame::BackendSFML)
endif ()
if (TARGET AntDashboardTests)
    target_link_libraries(AntDashboardTests PRIVATE PipeFrame::BackendSFML)
endif ()
