cmake_minimum_required(VERSION 4.1)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif ()

project(PipeFrame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(PIPEFRAME_BUILD_QT_EDITOR "Build the experimental Qt editor" OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_subdirectory(tools/PipeFrameHeaderTool)
add_subdirectory(engine)
add_subdirectory(editor)

file(GLOB PIPEFRAME_PROJECT_SOURCE_DIRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/projects/*/Source"
)

foreach(PIPEFRAME_PROJECT_SOURCE_DIR ${PIPEFRAME_PROJECT_SOURCE_DIRS})
    if(EXISTS "${PIPEFRAME_PROJECT_SOURCE_DIR}/CMakeLists.txt")
        add_subdirectory("${PIPEFRAME_PROJECT_SOURCE_DIR}")
    endif()
endforeach()

add_subdirectory(apps/PipeFrameEditor)

if (PIPEFRAME_BUILD_QT_EDITOR)
    add_subdirectory(apps/PipeFrameQtEditor)
endif()
