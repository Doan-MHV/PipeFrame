file(GLOB_RECURSE PIPEFRAME_EDITOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_library(PipeFrameEditor ${PIPEFRAME_EDITOR_SOURCES})

target_include_directories(PipeFrameEditor PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(PipeFrameEditor PUBLIC
    PipeFrameEngine
    imgui::imgui
)
