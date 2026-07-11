add_subdirectory(core)
add_subdirectory(render)
add_subdirectory(playback)
add_subdirectory(ui)

add_executable(ve main.cpp)
target_link_libraries(ve PRIVATE ve_ui ve_playback ve_render ve_core)
