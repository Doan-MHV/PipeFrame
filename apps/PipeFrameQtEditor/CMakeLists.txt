find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)

file(GLOB_RECURSE PIPEFRAME_QT_EDITOR_APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(PipeFrameQtEditor ${PIPEFRAME_QT_EDITOR_APP_SOURCES})

set_target_properties(PipeFrameQtEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_link_libraries(PipeFrameQtEditor PRIVATE
    Qt6::Widgets
)
