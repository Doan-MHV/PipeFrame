file(GLOB_RECURSE PIPEFRAME_FlappyBird_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

set(PIPEFRAME_FlappyBird_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/Generated")
set(PIPEFRAME_FlappyBird_GENERATED_HEADER
    "${PIPEFRAME_FlappyBird_GENERATED_DIR}/ProjectComponents.generated.h"
)

add_custom_command(
    OUTPUT "${PIPEFRAME_FlappyBird_GENERATED_HEADER}"
    COMMAND PipeFrameHeaderTool
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PIPEFRAME_FlappyBird_GENERATED_HEADER}"
    DEPENDS
        PipeFrameHeaderTool
        ${PIPEFRAME_FlappyBird_SOURCES}
    COMMENT "Generating FlappyBird component metadata"
    VERBATIM
)

add_library(PipeFrameFlappyBird SHARED
    ${PIPEFRAME_FlappyBird_SOURCES}
    "${PIPEFRAME_FlappyBird_GENERATED_HEADER}"
)

target_include_directories(PipeFrameFlappyBird PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(PipeFrameFlappyBird PRIVATE
    PipeFrameEngine
)
