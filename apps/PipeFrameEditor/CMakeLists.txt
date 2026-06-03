file(GLOB_RECURSE PIPEFRAME_EDITOR_APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(PipeFrame ${PIPEFRAME_EDITOR_APP_SOURCES})

set_target_properties(PipeFrame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    ENABLE_EXPORTS ON
)

target_include_directories(PipeFrame PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(PipeFrame PRIVATE
    PipeFrameEditor
)

target_compile_definitions(PipeFrame PRIVATE
    PIPEFRAME_BUILD_DIR="${CMAKE_BINARY_DIR}"
    PIPEFRAME_DEFAULT_PROJECT_FILE="${CMAKE_SOURCE_DIR}/projects/JungleDemo/PipeFrameProject.json"
    PIPEFRAME_PROJECTS_ROOT="${CMAKE_SOURCE_DIR}/projects"
    PIPEFRAME_TEMPLATE_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/templates"
    PIPEFRAME_SAMPLE_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/templates/SampleAssets"
)
