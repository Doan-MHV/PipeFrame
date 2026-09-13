add_executable(ThermalLab Main.cpp)
target_link_libraries(ThermalLab PRIVATE PipeFrame::Engine)
target_compile_definitions(ThermalLab PRIVATE
    PIPEFRAME_THERMAL_FONT="${PROJECT_SOURCE_DIR}/assets/fonts/roboto_regular.ttf"
    PIPEFRAME_THERMAL_GOLDENS="${CMAKE_CURRENT_SOURCE_DIR}/goldens")
if(BUILD_TESTING)
    add_test(NAME ThermalLabGoldenAcceptance COMMAND ThermalLab --check)
endif()

# Native hosts/tests opt in explicitly; Engine no longer exports backend dependencies.
if(TARGET ThermalLab)
    target_link_libraries(ThermalLab PRIVATE PipeFrame::BackendSFML)
endif()
