cmake_minimum_required(VERSION 3.16)

project(Screencap)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenCV REQUIRED)
# find_package(OpenCV REQUIRED core imgproc imgcodecs)
include_directories(${OpenCV_INCLUDE_DIRS})
# build dll
add_library(Screencap SHARED screencap.cpp utils.cpp)
target_compile_definitions(Screencap PRIVATE BUILD_DLL)
# build exe
# add_executable(Screencap screencap.cpp utils.cpp) # target_compile_definitions(Screencap PRIVATE BUILD_EXE)
# test exe
# add_executable(Screencap screencap.cpp utils.cpp)
# target_compile_definitions(Screencap PRIVATE BUILD_EXE_TEST)

target_compile_options(Screencap PRIVATE /await)
target_include_directories(Screencap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
target_link_libraries(Screencap PRIVATE 
    User32.lib
    D3D11.lib
    windowsapp.lib
    # opencv_core
    # opencv_imgproc
    # opencv_imgcodecs
    ${OpenCV_LIBS}
)
add_executable(ScreencapTest template_match.cpp utils.cpp)

target_include_directories(ScreencapTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
target_link_libraries(ScreencapTest PRIVATE 
    User32.lib
    D3D11.lib
    windowsapp.lib
    ${OpenCV_LIBS}
)