add_executable(UIGallery Main.cpp ControlsGallery.cpp)
target_link_libraries(UIGallery PRIVATE PipeFrame::Engine)
target_compile_definitions(UIGallery PRIVATE
        PIPEFRAME_GALLERY_FONT="${PROJECT_SOURCE_DIR}/assets/fonts/roboto_regular.ttf")
if (BUILD_TESTING)
    add_test(NAME UIGalleryAcceptance COMMAND UIGallery --check)
endif ()

# Native hosts/tests opt in explicitly; Engine no longer exports backend dependencies.
if(TARGET UIGallery)
    target_link_libraries(UIGallery PRIVATE PipeFrame::BackendSFML)
endif()
