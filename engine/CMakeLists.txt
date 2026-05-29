file(GLOB_RECURSE PIPEFRAME_ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl"
)

set(PIPEFRAME_ENGINE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/Generated")
set(PIPEFRAME_ENGINE_GENERATED_HEADER
    "${PIPEFRAME_ENGINE_GENERATED_DIR}/EngineComponents.generated.h"
)

add_custom_command(
    OUTPUT "${PIPEFRAME_ENGINE_GENERATED_HEADER}"
    COMMAND PipeFrameHeaderTool
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${PIPEFRAME_ENGINE_GENERATED_HEADER}"
        "RegisterGeneratedEngineComponents"
    DEPENDS
        PipeFrameHeaderTool
        ${PIPEFRAME_ENGINE_SOURCES}
    COMMENT "Generating engine component metadata"
    VERBATIM
)

add_library(PipeFrameEngine SHARED
    ${PIPEFRAME_ENGINE_SOURCES}
    "${PIPEFRAME_ENGINE_GENERATED_HEADER}"
)

target_include_directories(PipeFrameEngine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "$<TARGET_PROPERTY:SDL3::SDL3,INTERFACE_INCLUDE_DIRECTORIES>"
    "$<TARGET_PROPERTY:SDL3_ttf::SDL3_ttf,INTERFACE_INCLUDE_DIRECTORIES>"
    "$<TARGET_PROPERTY:SDL3_image::SDL3_image,INTERFACE_INCLUDE_DIRECTORIES>"
    "$<TARGET_PROPERTY:glm::glm,INTERFACE_INCLUDE_DIRECTORIES>"
    "$<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>"
)

target_link_libraries(PipeFrameEngine PRIVATE
    SDL3::SDL3
    $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>
    SDL3_ttf::SDL3_ttf
)

target_link_libraries(PipeFrameEngine PUBLIC
    glm::glm
    nlohmann_json::nlohmann_json
)
