add_library(ve_playback STATIC
    playback_engine.cpp
)
target_include_directories(ve_playback PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ve_playback PUBLIC Qt6::Core Qt6::Multimedia ve_core)
