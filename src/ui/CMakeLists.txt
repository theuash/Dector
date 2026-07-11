add_library(ve_ui STATIC
    main_window.cpp
    media_browser.cpp
    viewer_widget.cpp
    timeline_widget.cpp
    time_ruler.cpp
)
target_include_directories(ve_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ve_ui PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGLWidgets ve_core)
