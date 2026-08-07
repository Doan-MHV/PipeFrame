find_package(SFML CONFIG REQUIRED COMPONENTS Graphics Window System)

add_library(PipeFrameEngine
        src/Core/Application.cpp
        src/Core/Time.cpp
        src/Render/Camera2D.cpp
        src/Platform/Input.cpp
)

target_include_directories(PipeFrameEngine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PipeFrameEngine
        PUBLIC
        SFML::Graphics
        SFML::Window
        SFML::System
)