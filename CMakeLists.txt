cmake_minimum_required(VERSION 3.20)
project(ve VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets Multimedia)
find_package(nlohmann_json REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC IMPORTED_TARGET libavcodec)
pkg_check_modules(AVFORMAT IMPORTED_TARGET libavformat)
pkg_check_modules(AVUTIL IMPORTED_TARGET libavutil)
pkg_check_modules(SWSCALE IMPORTED_TARGET libswscale)
pkg_check_modules(SWRESAMPLE IMPORTED_TARGET libswresample)

add_subdirectory(src)
