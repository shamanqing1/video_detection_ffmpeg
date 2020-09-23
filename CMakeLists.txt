cmake_minimum_required(VERSION 3.10)
project(encode_decode VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(OpenCV REQUIRED)

add_executable(decode decode.cpp)
target_link_libraries(decode PUBLIC avcodec avformat avutil swscale
                      opencv_core opencv_imgcodecs opencv_imgproc
                      boost_filesystem)

add_executable(encode encode.cpp)
target_link_libraries(encode PUBLIC avcodec avformat avutil swscale
                      opencv_core opencv_imgcodecs opencv_imgproc
                      boost_filesystem)

add_executable(encode_format encode_format.cpp)
target_link_libraries(encode_format PUBLIC avcodec avformat avutil swscale
                      opencv_core opencv_imgcodecs opencv_imgproc
                      boost_filesystem)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
