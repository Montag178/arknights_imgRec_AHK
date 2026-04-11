cmake_minimum_required(VERSION 3.16)

project(Screencap)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(OpenCV REQUIRED)

# build dll
add_library(Screencap SHARED screencap.cpp utils.cpp MatchingMethods.cpp)
target_compile_definitions(Screencap PRIVATE BUILD_DLL)
# # build exe
# # add_executable(Screencap screencap.cpp utils.cpp) # target_compile_definitions(Screencap PRIVATE BUILD_EXE)

target_compile_options(Screencap PRIVATE /await)

target_include_directories(Screencap PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(Screencap PRIVATE 
    User32.lib
    D3D11.lib
    windowsapp.lib
    ${OpenCV_LIBS}
)