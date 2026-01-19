cmake_minimum_required(VERSION 4.1)


if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

project(PipeFrame)

set(CMAKE_CXX_STANDARD 20)


file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
add_executable(PipeFrame ${SOURCES})

find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(PipeFrame PRIVATE SDL3::SDL3)

find_package(SDL3_image CONFIG REQUIRED)
target_link_libraries(PipeFrame PRIVATE $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>)

find_package(SDL3_ttf CONFIG REQUIRED)
target_link_libraries(PipeFrame PRIVATE SDL3_ttf::SDL3_ttf)

find_package(glm CONFIG REQUIRED)
target_link_libraries(PipeFrame PRIVATE glm::glm)

add_custom_command(TARGET PipeFrame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:PipeFrame>/assets
)
