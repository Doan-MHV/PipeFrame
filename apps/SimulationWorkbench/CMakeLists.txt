add_executable(SimulationWorkbench
        Main.cpp
        Backend/SFML/Workbench.cpp
        Backend/SFML/DiagnosticsOverlay.cpp

        Editor/HierarchyPanel.cpp
        Editor/WorkspaceToolsPanel.cpp
        Editor/AssetBrowserPanel.cpp
        Editor/InspectorPanel.cpp
        Editor/ProjectManager.cpp
        Editor/ProjectScaffolder.cpp
        Editor/PrefabLibrary.cpp
        Editor/ProjectSession.cpp
        Editor/SceneDocument.cpp
        Editor/SceneHistory.cpp
        Editor/SceneSerializer.cpp
        Editor/ViewportToolbar.cpp
        Editor/WorkbenchInput.cpp
        Backend/SFML/WorkbenchLayout.cpp
        Backend/SFML/WorkbenchView.cpp
        Editor/ProjectBrowser.cpp

        Runtime/ProjectRuntimeHost.cpp
        Runtime/SimulationSession.cpp

        Platform/NativeProjectDialog.cpp
)

find_package(nfd CONFIG REQUIRED)

target_link_libraries(SimulationWorkbench
        PRIVATE
        PipeFrame::Engine
        nfd::nfd
)

target_compile_definitions(SimulationWorkbench
        PRIVATE
        PIPEFRAME_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
        PIPEFRAME_SDK_DIR="${CMAKE_SOURCE_DIR}"
        PIPEFRAME_HOST_BUILD_DIR="${CMAKE_BINARY_DIR}"
        PIPEFRAME_HOST_CONFIGURATION="$<CONFIG>"
        PIPEFRAME_CMAKE_COMMAND="${CMAKE_COMMAND}"
        PIPEFRAME_ENGINE_LIBRARY="$<TARGET_FILE:PipeFrameEngine>"
)

if (BUILD_TESTING)
    add_library(WorkbenchTestRuntime SHARED tests/WorkbenchTestRuntime.cpp)
    target_link_libraries(WorkbenchTestRuntime PRIVATE PipeFrame::Engine)
    add_library(IncompatibleWorkbenchRuntime SHARED tests/IncompatibleWorkbenchRuntime.cpp)
    target_link_libraries(IncompatibleWorkbenchRuntime PRIVATE PipeFrame::Engine)
    add_library(OldAbiWorkbenchRuntime SHARED tests/IncompatibleWorkbenchRuntime.cpp)
    target_link_libraries(OldAbiWorkbenchRuntime PRIVATE PipeFrame::Engine)
    target_compile_definitions(OldAbiWorkbenchRuntime PRIVATE PIPEFRAME_TEST_OLD_ABI)
    add_executable(WorkbenchUITests
            tests/WorkbenchUITests.cpp Backend/SFML/DiagnosticsOverlay.cpp
            Editor/HierarchyPanel.cpp Editor/WorkspaceToolsPanel.cpp Editor/AssetBrowserPanel.cpp Editor/InspectorPanel.cpp Editor/ViewportToolbar.cpp
            Backend/SFML/WorkbenchLayout.cpp Backend/SFML/WorkbenchView.cpp Editor/WorkbenchInput.cpp Editor/ProjectBrowser.cpp
            Editor/ProjectSession.cpp Editor/ProjectManager.cpp Editor/SceneDocument.cpp
            Editor/ProjectScaffolder.cpp
            Editor/SceneHistory.cpp Editor/SceneSerializer.cpp Editor/PrefabLibrary.cpp Runtime/ProjectRuntimeHost.cpp Runtime/SimulationSession.cpp)
    target_link_libraries(WorkbenchUITests PRIVATE PipeFrame::Engine)
    target_compile_definitions(WorkbenchUITests PRIVATE
            PIPEFRAME_TEST_FONT="${PROJECT_SOURCE_DIR}/assets/fonts/roboto_regular.ttf"
            PIPEFRAME_TEST_RUNTIME="$<TARGET_FILE:WorkbenchTestRuntime>"
            PIPEFRAME_OLD_ABI_RUNTIME="$<TARGET_FILE:OldAbiWorkbenchRuntime>"
            PIPEFRAME_INCOMPATIBLE_RUNTIME="$<TARGET_FILE:IncompatibleWorkbenchRuntime>"
            PIPEFRAME_BASIC_PROJECT="${PROJECT_SOURCE_DIR}/examples/BasicSimulation/project.pipeframe"
            PIPEFRAME_ANT_PROJECT="${PROJECT_SOURCE_DIR}/examples/AntSimulation/project.pipeframe"
            PIPEFRAME_SAILBOAT_PROJECT="${PROJECT_SOURCE_DIR}/examples/SailBoatSimulation/project.pipeframe")
    add_dependencies(WorkbenchUITests WorkbenchTestRuntime IncompatibleWorkbenchRuntime OldAbiWorkbenchRuntime)
    add_test(NAME WorkbenchUIAcceptance COMMAND WorkbenchUITests)

    add_executable(SceneSerializerTests
            tests/SceneSerializerTests.cpp
            Editor/SceneDocument.cpp
            Editor/SceneSerializer.cpp
    )

    target_link_libraries(SceneSerializerTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME SceneSerializerRoundTrip
            COMMAND SceneSerializerTests
    )

    add_executable(ProjectManagerTests
            tests/ProjectManagerTests.cpp
            Editor/ProjectManager.cpp
            Editor/ProjectScaffolder.cpp
            Editor/SceneDocument.cpp
            Editor/SceneSerializer.cpp
    )

    target_link_libraries(ProjectManagerTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME ProjectManagerRegression
            COMMAND ProjectManagerTests
    )

    add_executable(SceneHistoryTests
            tests/SceneHistoryTests.cpp
            Editor/SceneDocument.cpp
            Editor/SceneHistory.cpp
    )

    target_link_libraries(SceneHistoryTests
            PRIVATE
            PipeFrame::Engine
    )

    add_test(
            NAME SceneHistoryRegression
            COMMAND SceneHistoryTests
    )

    add_executable(AntComponentEditingTests tests/AntComponentEditingTests.cpp
        Editor/ProjectSession.cpp Editor/ProjectManager.cpp Editor/ProjectScaffolder.cpp
        Editor/SceneDocument.cpp Editor/SceneHistory.cpp Editor/SceneSerializer.cpp Editor/PrefabLibrary.cpp
        Editor/InspectorPanel.cpp Runtime/ProjectRuntimeHost.cpp Runtime/SimulationSession.cpp)
    target_link_libraries(AntComponentEditingTests PRIVATE PipeFrame::Engine)
    target_compile_definitions(AntComponentEditingTests PRIVATE
        PIPEFRAME_TEST_FONT="${PROJECT_SOURCE_DIR}/assets/fonts/roboto_regular.ttf"
        PIPEFRAME_ANT_RUNTIME="$<TARGET_FILE:AntSimulationRuntime>"
        PIPEFRAME_ANT_ASSETS="${PROJECT_SOURCE_DIR}/examples/AntSimulation/Assets")
    add_dependencies(AntComponentEditingTests AntSimulationRuntime)
    add_test(NAME AntComponentEditingAcceptance COMMAND AntComponentEditingTests)

    add_executable(RuntimeSceneEditTests
            tests/RuntimeSceneEditTests.cpp
            Editor/ProjectSession.cpp
            Editor/ProjectManager.cpp
            Editor/ProjectScaffolder.cpp
            Editor/SceneDocument.cpp
            Editor/SceneHistory.cpp
            Editor/SceneSerializer.cpp
            Editor/PrefabLibrary.cpp
            Runtime/ProjectRuntimeHost.cpp
    )

    target_link_libraries(RuntimeSceneEditTests PRIVATE PipeFrame::Engine)

    add_test(
            NAME RuntimeSceneEditRegression
            COMMAND RuntimeSceneEditTests
    )

    add_executable(SceneModelTests
            tests/SceneModelTests.cpp
            Editor/ProjectSession.cpp
            Editor/ProjectManager.cpp
            Editor/ProjectScaffolder.cpp
            Editor/SceneDocument.cpp
            Editor/SceneHistory.cpp
            Editor/SceneSerializer.cpp
            Editor/SceneWorkspace.cpp
            Editor/PrefabLibrary.cpp
            Runtime/ProjectRuntimeHost.cpp
    )
    target_link_libraries(SceneModelTests PRIVATE PipeFrame::Engine)
    add_test(NAME ComponentSceneModelAcceptance COMMAND SceneModelTests)

    add_executable(PrefabLibraryTests
            tests/PrefabLibraryTests.cpp
            Editor/PrefabLibrary.cpp
            Editor/SceneDocument.cpp
            Editor/SceneHistory.cpp
            Editor/SceneSerializer.cpp)
    target_link_libraries(PrefabLibraryTests PRIVATE PipeFrame::Engine)
    add_test(NAME PrefabLibraryRegression COMMAND PrefabLibraryTests)
endif()

if(BUILD_TESTING)
    add_executable(ProjectBuildTests tests/ProjectBuildTests.cpp
        Editor/ProjectSession.cpp Editor/ProjectManager.cpp Editor/ProjectScaffolder.cpp
        Editor/SceneDocument.cpp Editor/SceneHistory.cpp Editor/SceneSerializer.cpp
        Editor/PrefabLibrary.cpp Runtime/ProjectRuntimeHost.cpp)
    target_link_libraries(ProjectBuildTests PRIVATE PipeFrame::Engine)
    target_compile_definitions(ProjectBuildTests PRIVATE
        PIPEFRAME_SDK_DIR="${CMAKE_SOURCE_DIR}"
        PIPEFRAME_HOST_BUILD_DIR="${CMAKE_BINARY_DIR}"
        PIPEFRAME_HOST_CONFIGURATION="$<CONFIG>"
        PIPEFRAME_CMAKE_COMMAND="${CMAKE_COMMAND}"
        PIPEFRAME_ENGINE_LIBRARY="$<TARGET_FILE:PipeFrameEngine>")
    add_test(NAME ProjectBuildAcceptance COMMAND ProjectBuildTests)
    set_tests_properties(ProjectBuildAcceptance PROPERTIES TIMEOUT 180)
endif()

if(BUILD_TESTING)
    add_executable(BlankProjectAcceptanceTests tests/BlankProjectAcceptanceTests.cpp
        Editor/ProjectSession.cpp Editor/ProjectManager.cpp Editor/ProjectScaffolder.cpp
        Editor/SceneDocument.cpp Editor/SceneHistory.cpp Editor/SceneSerializer.cpp
        Editor/PrefabLibrary.cpp Runtime/ProjectRuntimeHost.cpp Runtime/SimulationSession.cpp)
    target_link_libraries(BlankProjectAcceptanceTests PRIVATE PipeFrame::Engine PipeFrame::BackendSFML)
    target_compile_definitions(BlankProjectAcceptanceTests PRIVATE
        PIPEFRAME_SDK_DIR="${CMAKE_SOURCE_DIR}"
        PIPEFRAME_HOST_BUILD_DIR="${CMAKE_BINARY_DIR}"
        PIPEFRAME_HOST_CONFIGURATION="$<CONFIG>"
        PIPEFRAME_CMAKE_COMMAND="${CMAKE_COMMAND}"
        PIPEFRAME_ENGINE_LIBRARY="$<TARGET_FILE:PipeFrameEngine>")
    add_test(NAME BlankProjectAcceptance COMMAND BlankProjectAcceptanceTests)
    set_tests_properties(BlankProjectAcceptance PROPERTIES TIMEOUT 180)
endif()

if(BUILD_TESTING)
    # Real presenter sources must compile without transitive native include paths.
    add_library(WorkbenchPresenterBoundaryCompile OBJECT
        tests/WorkbenchInputBoundaryCompile.cpp
        Editor/HierarchyPanel.cpp Editor/InspectorPanel.cpp Editor/ViewportToolbar.cpp
        Editor/AssetBrowserPanel.cpp Editor/WorkspaceToolsPanel.cpp Editor/ProjectBrowser.cpp)
    target_include_directories(WorkbenchPresenterBoundaryCompile PRIVATE
        "${PROJECT_SOURCE_DIR}/engine/include" "${CMAKE_CURRENT_SOURCE_DIR}/Editor")
    target_compile_features(WorkbenchPresenterBoundaryCompile PRIVATE cxx_std_20)
endif()

# Native hosts/tests opt in explicitly; Engine no longer exports backend dependencies.
if(TARGET SimulationWorkbench)
    target_link_libraries(SimulationWorkbench PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET WorkbenchUITests)
    target_link_libraries(WorkbenchUITests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET AntComponentEditingTests)
    target_link_libraries(AntComponentEditingTests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET RuntimeSceneEditTests)
    target_link_libraries(RuntimeSceneEditTests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET SceneModelTests)
    target_link_libraries(SceneModelTests PRIVATE PipeFrame::BackendSFML)
endif()
if(TARGET ProjectBuildTests)
    target_link_libraries(ProjectBuildTests PRIVATE PipeFrame::BackendSFML)
endif()
