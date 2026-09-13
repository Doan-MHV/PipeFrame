if (NOT DEFINED PIPEFRAME_SOURCE_DIR)
    message(FATAL_ERROR "PIPEFRAME_SOURCE_DIR is required")
endif ()

set(neutral_headers
    engine/include/PipeFrame/Components/KinematicBody2DComponent.h
    engine/include/PipeFrame/Editor/TilemapAssetEditor.h
    engine/include/PipeFrame/Editor/TilemapPaintTool.h
    engine/include/PipeFrame/Components/TilemapComponent.h
    engine/include/PipeFrame/Components/PlaygroundComponent.h
    apps/SimulationWorkbench/Editor/WorkbenchInput.h
    engine/include/PipeFrame/UI/FloatingWindowController.h
    engine/include/PipeFrame/UI/DockResizeController.h
    engine/include/PipeFrame/UI/ViewPanel.h
    apps/SimulationWorkbench/Editor/HierarchyPanel.h
    apps/SimulationWorkbench/Editor/InspectorPanel.h
    apps/SimulationWorkbench/Editor/ViewportToolbar.h
    apps/SimulationWorkbench/Editor/AssetBrowserPanel.h
    apps/SimulationWorkbench/Editor/WorkspaceToolsPanel.h
    apps/SimulationWorkbench/Editor/ProjectBrowser.h

    engine/include/PipeFrame/UI/SimulationDashboard.h
    engine/include/PipeFrame/UI/ViewTheme.h
    engine/include/PipeFrame/Render/CameraController2D.h
    engine/include/PipeFrame/Render/Camera2D.h
    engine/include/PipeFrame/Render/RenderContext.h
    engine/include/PipeFrame/Foundation/MathTypes.h
    engine/include/PipeFrame/Input/InputEvent.h
    engine/include/PipeFrame/Render/RenderTypes.h
    engine/include/PipeFrame/Render/Canvas.h
    engine/include/PipeFrame/Core/ProcessTask.h
    apps/SimulationWorkbench/Runtime/ProjectRuntimeHost.h
    engine/include/PipeFrame/World/World.h
    engine/include/PipeFrame/UI/MountedView.h
    engine/include/PipeFrame/UI/ViewSource.h
    engine/include/PipeFrame/UI/SimulationTransportView.h
    engine/include/PipeFrame/UI/SchemaInspector.h
    engine/include/PipeFrame/UI/View.h
    engine/include/PipeFrame/Core/FixedStepSequence.h
    engine/include/PipeFrame/Components/EnergyComponent.h
    engine/include/PipeFrame/Components/EnergySchema.h
    engine/include/PipeFrame/Components/CommonComponentSchemas.h
    engine/include/PipeFrame/Components/RandomStateComponent.h
    engine/include/PipeFrame/Project/ComponentRegistry.h
    engine/include/PipeFrame/ECS/ComponentView.h
    engine/include/PipeFrame/ECS/SceneViewCache.h
    engine/include/PipeFrame/Components/Transform2DComponent.h
    engine/include/PipeFrame/Simulation/Steering2D.h
    engine/include/PipeFrame/Simulation/DirectionTracker.h
    engine/include/PipeFrame/ECS/Entity.h
    engine/include/PipeFrame/Entities/EntityArchetype.h
    engine/include/PipeFrame/UI/StatefulView.h
    engine/include/PipeFrame/ECS/World.h
    engine/include/PipeFrame/ECS/Scene.h
    examples/AntSimulation/Source/World/Rendering/AntGeometry.h
    examples/AntSimulation/Source/World/Rendering/AntGeometry.cpp
    examples/AntSimulation/Source/Editor/AntSelectionView.h
    engine/include/PipeFrame/UI/ViewModels.h
    engine/include/PipeFrame/Project/ProjectTypes.h
    engine/include/PipeFrame/Project/ProjectRuntime.h)

file(GLOB_RECURSE extracted_service_headers RELATIVE "${PIPEFRAME_SOURCE_DIR}"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Core/AsyncTask.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Core/DeterministicRandom.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Core/FixedStepScheduler.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Core/Profiler.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Core/Signal.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Core/ThreadPool.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Data/*.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Environment/*.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Learning/*.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Physics/*.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Resources/*.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Audio/*.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Render/RenderServices2D.h"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/Spatial/*.h")
list(APPEND neutral_headers ${extracted_service_headers})

# Audit the whole public SDK, including transitive legacy-widget dependents.
# Native opt-in adapters are the only permitted backend headers.
file(GLOB_RECURSE public_sdk_headers RELATIVE "${PIPEFRAME_SOURCE_DIR}"
    "${PIPEFRAME_SOURCE_DIR}/engine/include/PipeFrame/*.h")
list(FILTER public_sdk_headers EXCLUDE REGEX "/Backend/")
file(GLOB public_editor_headers RELATIVE "${PIPEFRAME_SOURCE_DIR}"
    "${PIPEFRAME_SOURCE_DIR}/apps/SimulationWorkbench/Editor/*.h")
list(APPEND neutral_headers ${public_sdk_headers} ${public_editor_headers})

foreach (relative_path IN LISTS neutral_headers)
    file(READ "${PIPEFRAME_SOURCE_DIR}/${relative_path}" contents)
    if (contents MATCHES "SFML|PipeFrame/Backend/|(^|[^A-Za-z0-9_])sf::")
        message(FATAL_ERROR "Backend type leaked into neutral public contract: ${relative_path}")
    endif ()
endforeach ()

if(NOT PIPEFRAME_ANT_REWORK_ONLY)
file(GLOB sailboat_local_learning
    "${PIPEFRAME_SOURCE_DIR}/examples/SailBoatSimulation/Source/Neat/*"
    "${PIPEFRAME_SOURCE_DIR}/examples/SailBoatSimulation/Source/Training/TrainingHistory.*")
if (sailboat_local_learning)
    message(FATAL_ERROR "Generic learning code must live in PipeFrame::Learning: ${sailboat_local_learning}")
endif ()

file(READ "${PIPEFRAME_SOURCE_DIR}/examples/SailBoatSimulation/CMakeLists.txt" sailboat_cmake)
if (sailboat_cmake MATCHES "Source/Neat|TrainingHistory\\.cpp")
    message(FATAL_ERROR "SailBoat still compiles a project-local generic learning implementation")
endif ()

endif()

file(STRINGS "${PIPEFRAME_SOURCE_DIR}/docs/parity/milestone17/SFML_MIGRATION_ALLOWLIST.txt"
     allowlist REGEX "^[^#].+")
file(GLOB_RECURSE example_sources RELATIVE "${PIPEFRAME_SOURCE_DIR}"
     "${PIPEFRAME_SOURCE_DIR}/examples/AntSimulation/Source/*.h"
     "${PIPEFRAME_SOURCE_DIR}/examples/AntSimulation/Source/*.hpp"
     "${PIPEFRAME_SOURCE_DIR}/examples/AntSimulation/Source/*.cpp"
     "${PIPEFRAME_SOURCE_DIR}/examples/SailBoatSimulation/Source/*.h"
     "${PIPEFRAME_SOURCE_DIR}/examples/SailBoatSimulation/Source/*.hpp"
     "${PIPEFRAME_SOURCE_DIR}/examples/SailBoatSimulation/Source/*.cpp")

if(PIPEFRAME_ANT_REWORK_ONLY)
    list(FILTER example_sources EXCLUDE REGEX "SailBoatSimulation")
    list(FILTER allowlist EXCLUDE REGEX "SailBoatSimulation")
endif()

set(observed)
foreach (relative_path IN LISTS example_sources)
    file(READ "${PIPEFRAME_SOURCE_DIR}/${relative_path}" contents)
    if (contents MATCHES "#include[ \t]*[<\"]SFML")
        list(APPEND observed "${relative_path}")
        if (NOT relative_path IN_LIST allowlist)
            message(FATAL_ERROR "New direct SFML include outside a migration adapter: ${relative_path}")
        endif ()
    endif ()
endforeach ()

foreach (relative_path IN LISTS allowlist)
    if (NOT relative_path IN_LIST observed)
        message(FATAL_ERROR "Stale SFML migration allowlist entry (remove it): ${relative_path}")
    endif ()
endforeach ()

file(GLOB_RECURSE example_headers RELATIVE "${PIPEFRAME_SOURCE_DIR}"
     "${PIPEFRAME_SOURCE_DIR}/examples/AntSimulation/Source/*.h"
     "${PIPEFRAME_SOURCE_DIR}/examples/AntSimulation/Source/*.hpp"
     "${PIPEFRAME_SOURCE_DIR}/examples/SailBoatSimulation/Source/*.h"
     "${PIPEFRAME_SOURCE_DIR}/examples/SailBoatSimulation/Source/*.hpp")

if(PIPEFRAME_ANT_REWORK_ONLY)
    list(FILTER example_headers EXCLUDE REGEX "SailBoatSimulation")
endif()
foreach (relative_path IN LISTS example_headers)
    file(READ "${PIPEFRAME_SOURCE_DIR}/${relative_path}" contents)
    if (contents MATCHES "sf::(Texture|Shader|RenderTexture|SoundBuffer|Sound|VertexArray|Font|Image|RenderWindow)[ \t\r\n]+[A-Za-z_]")
        message(FATAL_ERROR "Example owns an SFML resource instead of a PipeFrame handle: ${relative_path}")
    endif ()
    if (contents MATCHES "(unique_ptr|shared_ptr|optional|vector|array)[^;\r\n]*sf::(Texture|Shader|RenderTexture|SoundBuffer|Sound|VertexArray|Font|Image|RenderWindow)")
        message(FATAL_ERROR "Example container owns an SFML resource instead of a PipeFrame handle: ${relative_path}")
    endif ()
endforeach ()

foreach (project_cmake examples/AntSimulation/CMakeLists.txt examples/SailBoatSimulation/CMakeLists.txt)
    if(PIPEFRAME_ANT_REWORK_ONLY AND project_cmake MATCHES "SailBoatSimulation")
        continue()
    endif()
    file(READ "${PIPEFRAME_SOURCE_DIR}/${project_cmake}" contents)
    if (contents MATCHES "SFML::")
        message(FATAL_ERROR "Example links SFML directly instead of a PipeFrame module: ${project_cmake}")
    endif ()
endforeach ()

list(LENGTH observed adapter_count)
message(STATUS "PipeFrame dependency lint passed; ${adapter_count} recorded migration adapters remain")

# R5 presenters describe views. Native window/dock adapters are tracked separately by R6.
foreach (view_source
    examples/AntSimulation/Source/Editor/AntDashboard.cpp
    apps/SimulationWorkbench/Editor/ViewportToolbar.cpp
    apps/SimulationWorkbench/Editor/HierarchyPanel.cpp
    apps/SimulationWorkbench/Editor/InspectorPanel.cpp
    apps/SimulationWorkbench/Editor/AssetBrowserPanel.cpp
    apps/SimulationWorkbench/Editor/WorkspaceToolsPanel.cpp
    apps/SimulationWorkbench/Editor/ProjectBrowser.cpp)
    file(READ "${PIPEFRAME_SOURCE_DIR}/${view_source}" contents)
    if(contents MATCHES "CreateChild[<]|ReconcileChild[<]|ViewRenderer|[>]SetText[(]|[>]SetValue[(]")
        message(FATAL_ERROR "Declarative presenter uses imperative widget construction/synchronization: ${view_source}")
    endif()
endforeach()

# Official Ant schemas belong to component headers, never inline in registration.
file(READ "${PIPEFRAME_SOURCE_DIR}/examples/AntSimulation/Source/Runtime/AntRegistration.cpp" ant_registration_source)
if(ant_registration_source MATCHES "[.](Field|Editable|ReadOnly|Accessor|Validate)[(]")
    message(FATAL_ERROR "Ant registration must consume Component::Schema(); declare fields in the component header")
endif()

# Ant is the fully migrated reference: no project-owned native backend code.
file(GLOB_RECURSE ant_world_sources "${PIPEFRAME_SOURCE_DIR}/examples/AntSimulation/Source/*.h"
    "${PIPEFRAME_SOURCE_DIR}/examples/AntSimulation/Source/*.cpp")
foreach(source IN LISTS ant_world_sources)
    file(READ "${source}" contents)
    if(contents MATCHES "SFML|sf::|PipeFrame/Backend/")
        message(FATAL_ERROR "Ant project authoring must use neutral PipeFrame APIs: ${source}")
    endif()
endforeach()
