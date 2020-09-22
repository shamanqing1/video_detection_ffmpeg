cmake_minimum_required(VERSION 3.10)
project(decodec VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(OpenCV REQUIRED)

add_executable(decodec main.cpp)
target_link_libraries(decodec PUBLIC avcodec avformat avutil swscale
                      opencv_core opencv_imgcodecs opencv_imgproc)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
