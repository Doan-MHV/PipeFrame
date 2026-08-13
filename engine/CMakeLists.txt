find_package(
        SFML CONFIG REQUIRED
        COMPONENTS Graphics Window System
)

add_library(PipeFrameEngine
        src/Core/Application.cpp
        src/Core/Time.cpp
        src/Input/Input.cpp
        src/Render/Camera2D.cpp
        src/Simulation/SimulationController.cpp
)

add_library(PipeFrame::Engine ALIAS PipeFrameEngine)

target_compile_features(PipeFrameEngine
        PUBLIC
        cxx_std_20
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