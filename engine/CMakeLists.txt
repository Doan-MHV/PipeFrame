find_package(
        SFML CONFIG REQUIRED
        COMPONENTS Graphics Window System
)
find_package(Threads REQUIRED)

add_library(PipeFrameFoundation INTERFACE)
add_library(PipeFrame::Foundation ALIAS PipeFrameFoundation)
target_compile_features(PipeFrameFoundation INTERFACE cxx_std_20)
target_include_directories(PipeFrameFoundation INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(PipeFrameFoundation INTERFACE Threads::Threads)

add_library(PipeFrameBackendSFML INTERFACE)
add_library(PipeFrame::BackendSFML ALIAS PipeFrameBackendSFML)
target_link_libraries(PipeFrameBackendSFML INTERFACE SFML::Graphics SFML::Window SFML::System)

add_library(PipeFrameLearning STATIC
        src/Learning/DirectedAcyclicGraph.cpp
        src/Learning/Genome.cpp
        src/Learning/GenomeMutator.cpp
        src/Learning/GenomeSelector.cpp
        src/Learning/Network.cpp
        src/Learning/NetworkGenerator.cpp
        src/Learning/ExperimentRuntime.cpp
        src/Learning/EvolutionExperiment.cpp)
add_library(PipeFrame::Learning ALIAS PipeFrameLearning)
target_compile_features(PipeFrameLearning PUBLIC cxx_std_20)
target_include_directories(PipeFrameLearning PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(PipeFrameLearning PUBLIC PipeFrame::Foundation)
set_target_properties(PipeFrameLearning PROPERTIES POSITION_INDEPENDENT_CODE ON)

find_package(SFML CONFIG REQUIRED COMPONENTS Audio)
add_library(PipeFrameAudio STATIC src/Audio/AudioService.cpp)
add_library(PipeFrame::Audio ALIAS PipeFrameAudio)
target_compile_features(PipeFrameAudio PUBLIC cxx_std_20)
target_include_directories(PipeFrameAudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(PipeFrameAudio PUBLIC PipeFrame::Foundation PRIVATE SFML::Audio)

add_custom_target(PipeFrameDependencyLint
    COMMAND ${CMAKE_COMMAND}
        -DPIPEFRAME_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DPIPEFRAME_ANT_REWORK_ONLY=${PIPEFRAME_ANT_REWORK_ONLY}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DependencyLint.cmake
    VERBATIM)

add_library(PipeFrameEngine SHARED
        src/Core/Application.cpp
        src/Core/Time.cpp
        src/Core/ProcessTask.cpp
        src/Input/Input.cpp
        src/Backend/SFML/DockSplitter.cpp
        src/Backend/SFML/InputEventAdapter.cpp
        src/Backend/SFML/GraphicsResourceService.cpp
        src/Backend/SFML/GeometryRenderer.cpp
        src/Backend/SFML/ImageData.cpp
        src/UI/SimulationDashboard.cpp
        src/Render/Camera2D.cpp
        src/Backend/SFML/RenderContextAdapter.cpp
        src/Simulation/SimulationController.cpp
        src/Render/CameraController2D.cpp
        src/UI/ViewPanel.cpp
        src/UI/DeclarativeView.cpp
        src/Backend/SFML/UI/Widget.cpp
        src/Backend/SFML/UI/Panel.cpp
        src/Backend/SFML/UI/RoundedRectangleShape.cpp
        src/Backend/SFML/UI/Surface.cpp
        src/Backend/SFML/UI/UIManager.cpp
        src/Backend/SFML/UI/Button.cpp
        src/Backend/SFML/UI/Badge.cpp
        src/Backend/SFML/UI/Tooltip.cpp
        src/Backend/SFML/UI/ModalBarrier.cpp
        src/Backend/SFML/UI/PopupLayer.cpp
        src/Backend/SFML/UI/Toast.cpp
        src/Backend/SFML/UI/ZenModeShell.cpp
        src/Backend/SFML/UI/TabView.cpp
        src/Backend/SFML/UI/ListView.cpp
        src/Backend/SFML/UI/ProgressBar.cpp
        src/Backend/SFML/UI/Gauge.cpp
        src/Backend/SFML/UI/Chart.cpp
        src/Backend/SFML/UI/NetworkView.cpp
        src/Backend/SFML/UI/TableView.cpp
        src/Backend/SFML/UI/CollapsiblePanel.cpp
        src/Backend/SFML/UI/MetricCard.cpp
        src/Backend/SFML/UI/EdgeDrawer.cpp
        src/Backend/SFML/UI/Toggle.cpp
        src/Backend/SFML/UI/Slider.cpp
        src/Backend/SFML/UI/SegmentedControl.cpp
        src/Backend/SFML/UI/Label.cpp
        src/Backend/SFML/UI/StackPanel.cpp
        src/Backend/SFML/UI/OverlayPanel.cpp
        src/Backend/SFML/UI/SimulationTransport.cpp
        src/Backend/SFML/UI/NumericField.cpp
        src/Backend/SFML/UI/TextButton.cpp
        src/Backend/SFML/UI/LabeledNumericField.cpp
        src/Backend/SFML/UI/ScrollPanel.cpp
        src/Project/ProjectRuntimeLibrary.cpp
        src/Project/AssetDatabase.cpp
        src/Project/EditorWorkspace.cpp
        src/Project/PluginAPI.cpp
        src/Backend/SFML/UI/TextField.cpp
        src/Backend/SFML/UI/LabeledTextField.cpp
)

add_library(PipeFrame::Engine ALIAS PipeFrameEngine)

set_target_properties(
        PipeFrameEngine
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)

target_compile_features(PipeFrameEngine
        PUBLIC
        cxx_std_20
)

target_include_directories(PipeFrameEngine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PipeFrameEngine
        PUBLIC
        PipeFrame::Foundation
        ${CMAKE_DL_LIBS}
        PRIVATE PipeFrame::BackendSFML
)

target_compile_definitions(PipeFrameEngine PRIVATE PIPEFRAME_RUNTIME_CONFIGURATION="$<CONFIG>")

if (BUILD_TESTING)
    add_test(NAME PipeFrameDependencyLint
        COMMAND ${CMAKE_COMMAND}
            -DPIPEFRAME_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DPIPEFRAME_ANT_REWORK_ONLY=${PIPEFRAME_ANT_REWORK_ONLY}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DependencyLint.cmake)
    add_executable(TilemapTests tests/TilemapTests.cpp)
    target_link_libraries(TilemapTests PRIVATE PipeFrame::Foundation)
    add_test(NAME TilemapRegression COMMAND TilemapTests)
    add_executable(VisualAssetTests tests/VisualAssetTests.cpp)
    target_link_libraries(VisualAssetTests PRIVATE PipeFrame::Engine PipeFrame::BackendSFML)
    add_test(NAME VisualAssetAcceptance COMMAND VisualAssetTests)
    add_executable(EnvironmentCollisionTests tests/EnvironmentCollisionTests.cpp)
    target_link_libraries(EnvironmentCollisionTests PRIVATE PipeFrame::Engine)
    add_test(NAME EnvironmentCollisionAcceptance COMMAND EnvironmentCollisionTests)
    add_executable(BrushToolTests tests/BrushToolTests.cpp)
    target_link_libraries(BrushToolTests PRIVATE PipeFrame::Engine)
    add_test(NAME CustomBrushAcceptance COMMAND BrushToolTests)
    add_executable(EnvironmentEditingTests tests/EnvironmentEditingTests.cpp)
    target_link_libraries(EnvironmentEditingTests PRIVATE PipeFrame::Engine)
    add_test(NAME EnvironmentEditingAcceptance COMMAND EnvironmentEditingTests)
    add_executable(DockResizeTests tests/DockResizeTests.cpp)
    target_link_libraries(DockResizeTests PRIVATE PipeFrame::Foundation)
    add_test(NAME DockResizeRegression COMMAND DockResizeTests)
    add_executable(FoundationHeadlessTests tests/FoundationHeadlessTests.cpp)
    target_link_libraries(FoundationHeadlessTests PRIVATE PipeFrame::Foundation)
    add_test(NAME FoundationHeadlessRegression COMMAND FoundationHeadlessTests)
    add_executable(AuthoringFoundationTests tests/AuthoringFoundationTests.cpp)
    target_link_libraries(AuthoringFoundationTests PRIVATE PipeFrame::Foundation)
    add_test(NAME AuthoringFoundationRegression COMMAND AuthoringFoundationTests)
    add_executable(AssetDatabaseTests tests/AssetDatabaseTests.cpp src/Project/AssetDatabase.cpp src/Backend/SFML/ImageData.cpp)
    target_link_libraries(AssetDatabaseTests PRIVATE PipeFrame::Foundation PipeFrame::BackendSFML)
    add_test(NAME AssetDatabaseRegression COMMAND AssetDatabaseTests)
    add_executable(EditorWorkspaceTests tests/EditorWorkspaceTests.cpp src/Project/EditorWorkspace.cpp)
    target_link_libraries(EditorWorkspaceTests PRIVATE PipeFrame::Foundation)
    add_test(NAME EditorWorkspaceRegression COMMAND EditorWorkspaceTests)
    add_executable(PluginAPITests tests/PluginAPITests.cpp)
    target_link_libraries(PluginAPITests PRIVATE PipeFrame::Engine)
    add_test(NAME PluginAPIRegression COMMAND PluginAPITests)
    add_executable(SpatialServicesTests tests/SpatialServicesTests.cpp)
    target_link_libraries(SpatialServicesTests PRIVATE PipeFrame::Foundation)
    add_test(NAME SpatialServicesRegression COMMAND SpatialServicesTests)
    add_executable(PhysicsEnvironmentTests tests/PhysicsEnvironmentTests.cpp)
    target_link_libraries(PhysicsEnvironmentTests PRIVATE PipeFrame::Foundation)
    add_test(NAME PhysicsEnvironmentRegression COMMAND PhysicsEnvironmentTests)
    add_executable(ResourceRenderTests tests/ResourceRenderTests.cpp)
    target_link_libraries(ResourceRenderTests PRIVATE PipeFrame::Foundation PipeFrame::Audio)
    add_test(NAME ResourceRenderRegression COMMAND ResourceRenderTests)
    add_executable(LearningExperimentTests tests/LearningExperimentTests.cpp)
    target_link_libraries(LearningExperimentTests PRIVATE PipeFrame::Learning)
    add_test(NAME LearningExperimentRegression COMMAND LearningExperimentTests)
    add_executable(RuntimeLibraryResolutionTests tests/RuntimeLibraryResolutionTests.cpp)
    target_link_libraries(RuntimeLibraryResolutionTests PRIVATE PipeFrame::Engine)
    target_compile_definitions(RuntimeLibraryResolutionTests PRIVATE PIPEFRAME_TEST_CONFIGURATION="$<CONFIG>")
    add_test(NAME RuntimeLibraryResolutionRegression COMMAND RuntimeLibraryResolutionTests)

    add_executable(UIHardeningTests tests/UIHardeningTests.cpp)
    target_link_libraries(UIHardeningTests PRIVATE PipeFrame::Engine)
    target_compile_definitions(UIHardeningTests PRIVATE PIPEFRAME_HARDENING_FONT="${PROJECT_SOURCE_DIR}/assets/fonts/roboto_regular.ttf")
    add_test(NAME UIHardeningRegression COMMAND UIHardeningTests)

    add_executable(TableViewTests tests/TableViewTests.cpp)
    target_link_libraries(TableViewTests PRIVATE PipeFrame::Engine)
    target_compile_definitions(TableViewTests PRIVATE
            PIPEFRAME_TABLE_TEST_FONT="${PROJECT_SOURCE_DIR}/assets/fonts/roboto_regular.ttf")
    add_test(NAME TableViewRegression COMMAND TableViewTests)

    add_executable(NetworkViewTests tests/NetworkViewTests.cpp)
    target_link_libraries(NetworkViewTests PRIVATE PipeFrame::Engine)
    add_test(NAME NetworkViewRegression COMMAND NetworkViewTests)

    add_executable(ChartTests tests/ChartTests.cpp)
    target_link_libraries(ChartTests PRIVATE PipeFrame::Engine)
    add_test(NAME ChartRegression COMMAND ChartTests)

    add_executable(ComponentRegistryTests tests/ComponentRegistryTests.cpp)
    target_link_libraries(ComponentRegistryTests PRIVATE PipeFrame::Engine)
    add_test(NAME ComponentRegistryAcceptance COMMAND ComponentRegistryTests)
    add_executable(SpritePhysicsTests tests/SpritePhysicsTests.cpp)
    target_link_libraries(SpritePhysicsTests PRIVATE PipeFrame::Engine)
    add_test(NAME SpritePhysicsAcceptance COMMAND SpritePhysicsTests)

    add_executable(SceneProjectRuntimeTests tests/SceneProjectRuntimeTests.cpp)
    target_link_libraries(SceneProjectRuntimeTests PRIVATE PipeFrame::Engine)
    add_test(NAME SceneProjectRuntimeAcceptance COMMAND SceneProjectRuntimeTests)

    add_executable(MountedViewTests tests/MountedViewTests.cpp)
    target_link_libraries(MountedViewTests PRIVATE PipeFrame::Engine)
    target_compile_definitions(MountedViewTests PRIVATE PIPEFRAME_TEST_FONT="${PROJECT_SOURCE_DIR}/assets/fonts/roboto_regular.ttf")
    add_test(NAME MountedViewRegression COMMAND MountedViewTests)

    add_executable(UIFrameworkTests
            tests/UIFrameworkTests.cpp
    )

    target_link_libraries(UIFrameworkTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME UIFrameworkRegression
            COMMAND UIFrameworkTests
    )

    add_executable(SimulationControllerTests
            tests/SimulationControllerTests.cpp
    )

    target_link_libraries(SimulationControllerTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME SimulationControllerRegression
            COMMAND SimulationControllerTests
    )
    add_executable(BehaviourServicesTests tests/BehaviourServicesTests.cpp)
    target_link_libraries(BehaviourServicesTests PRIVATE PipeFrame::Engine)
    add_test(NAME BehaviourServicesAcceptance COMMAND BehaviourServicesTests)

    add_executable(IndependentAuthoringTests tests/IndependentAuthoringTests.cpp)
    target_link_libraries(IndependentAuthoringTests PRIVATE PipeFrame::Engine PipeFrame::BackendSFML)
    target_compile_definitions(IndependentAuthoringTests PRIVATE PIPEFRAME_TEST_FONT="${PROJECT_SOURCE_DIR}/assets/fonts/roboto_regular.ttf")
    add_test(NAME IndependentAuthoringAcceptance COMMAND IndependentAuthoringTests)
    add_library(IndependentAuthoringPublicCompile OBJECT tests/IndependentAuthoringPublicCompile.cpp)
    target_include_directories(IndependentAuthoringPublicCompile PRIVATE include)
    target_compile_features(IndependentAuthoringPublicCompile PRIVATE cxx_std_20)

    add_executable(WorldModuleTests tests/WorldModuleTests.cpp)
    target_link_libraries(WorldModuleTests PRIVATE PipeFrame::Engine)
    add_test(NAME WorldModuleRegression COMMAND WorldModuleTests)
    # Compile without linking Engine: its native include directories must not leak here.
    add_library(RenderPublicHeadersTests OBJECT tests/RenderPublicHeadersTests.cpp)
    target_include_directories(RenderPublicHeadersTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_features(RenderPublicHeadersTests PRIVATE cxx_std_20)
    add_executable(RenderContextTests tests/RenderContextTests.cpp)
    target_link_libraries(RenderContextTests PRIVATE PipeFrame::Engine)
    add_test(NAME RenderContextRegression COMMAND RenderContextTests)
    add_executable(ProcessTaskTests tests/ProcessTaskTests.cpp)
    target_link_libraries(ProcessTaskTests PRIVATE PipeFrame::Engine)
    add_test(NAME ProcessTaskRegression COMMAND ProcessTaskTests)
endif ()

# Every public authoring header must stand on its own without native include paths.
if(BUILD_TESTING)
    file(GLOB public_ui_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/PipeFrame/UI/*.h")
    file(GLOB public_editor_headers CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/apps/SimulationWorkbench/Editor/*.h")
    set(isolated_header_sources)
    foreach(header IN LISTS public_ui_headers public_editor_headers)
        string(MD5 header_id "${header}")
        set(source "${CMAKE_CURRENT_BINARY_DIR}/isolated-ui/${header_id}.cpp")
        file(WRITE "${source}" "#include \"${header}\"\n")
        list(APPEND isolated_header_sources "${source}")
    endforeach()
    add_library(UIEditorPublicHeadersCompile OBJECT ${isolated_header_sources})
    target_include_directories(UIEditorPublicHeadersCompile PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_features(UIEditorPublicHeadersCompile PRIVATE cxx_std_20)
endif()

# Native hosts/tests opt in explicitly; Engine no longer exports backend dependencies.
if(TARGET AssetDatabaseTests)
    target_link_libraries(AssetDatabaseTests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET UIHardeningTests)
    target_link_libraries(UIHardeningTests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET TableViewTests)
    target_link_libraries(TableViewTests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET NetworkViewTests)
    target_link_libraries(NetworkViewTests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET ChartTests)
    target_link_libraries(ChartTests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET MountedViewTests)
    target_link_libraries(MountedViewTests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET UIFrameworkTests)
    target_link_libraries(UIFrameworkTests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET RenderContextTests)
    target_link_libraries(RenderContextTests PRIVATE PipeFrame::BackendSFML)
endif()

if(BUILD_TESTING)
    add_library(PublicEngineConsumerCompile OBJECT tests/PublicEngineConsumerCompile.cpp)
    target_link_libraries(PublicEngineConsumerCompile PRIVATE PipeFrame::Engine)
endif()
