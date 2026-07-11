add_library(ve_render STATIC
    media_decoder.cpp
    gl_renderer.cpp
    export_pipeline.cpp
)
target_include_directories(ve_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ve_render PUBLIC Qt6::Core Qt6::Gui Qt6::OpenGLWidgets Qt6::Multimedia ve_core
    ${AVCODEC_LIBRARIES}
    ${AVFORMAT_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SWSCALE_LIBRARIES}
    ${SWRESAMPLE_LIBRARIES}
)
