file(GLOB_RECURSE PIPEFRAME_EDITOR_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Core/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/EditorSessionState.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/EditorTypes.h"
)

file(GLOB_RECURSE PIPEFRAME_EDITOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

foreach(PIPEFRAME_EDITOR_CORE_SOURCE ${PIPEFRAME_EDITOR_CORE_SOURCES})
    list(REMOVE_ITEM PIPEFRAME_EDITOR_SOURCES "${PIPEFRAME_EDITOR_CORE_SOURCE}")
endforeach()

add_library(PipeFrameEditorCore ${PIPEFRAME_EDITOR_CORE_SOURCES})

target_include_directories(PipeFrameEditorCore PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(PipeFrameEditorCore PUBLIC
    PipeFrameEngine
)

find_package(imgui CONFIG REQUIRED)

add_library(PipeFrameEditor ${PIPEFRAME_EDITOR_SOURCES})

target_include_directories(PipeFrameEditor PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(PipeFrameEditor PUBLIC
    PipeFrameEditorCore
    PipeFrameEngine
    imgui::imgui
)
