add_library(ve_core STATIC
    rational_time.cpp
    asset.cpp
    parameter.cpp
    clip.cpp
    track.cpp
    sequence.cpp
    command.cpp
    project.cpp
)
target_include_directories(ve_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ve_core PUBLIC Qt6::Core Qt6::Gui nlohmann_json::nlohmann_json)
