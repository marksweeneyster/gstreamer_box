cmake_minimum_required(VERSION 3.28)
project(gstreamer_box C CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig)
find_package(Threads REQUIRED)

# May need to set environment variable PKG_CONFIG_PATH
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

add_library(GST_FLAGS INTERFACE)
target_include_directories(GST_FLAGS INTERFACE ${GSTREAMER_INCLUDE_DIRS})
target_link_directories(GST_FLAGS INTERFACE ${GSTREAMER_LIBRARY_DIRS})
target_link_libraries(GST_FLAGS INTERFACE ${GSTREAMER_LIBRARIES} Threads::Threads)


pkg_check_modules(GSTREAMER_PB REQUIRED gstreamer-pbutils-1.0)
# PB Utils includes all the base gstreamer settings
add_library(GST_PB INTERFACE)
target_include_directories(GST_PB INTERFACE ${GSTREAMER_PB_INCLUDE_DIRS})
target_link_directories(GST_PB INTERFACE ${GSTREAMER_PB_LIBRARY_DIRS})
target_link_libraries(GST_PB INTERFACE ${GSTREAMER_PB_LIBRARIES} Threads::Threads)

add_executable(gstreamer_box main.cpp)
target_link_libraries(gstreamer_box PUBLIC GST_FLAGS)

add_executable(simple_rtp simple_rtp.cpp)
target_link_libraries(simple_rtp PUBLIC GST_FLAGS)

add_executable(tutorial3 tutorial3.cpp)
target_link_libraries(tutorial3 PUBLIC GST_FLAGS)

add_executable(tutorial9 tutorial9.cpp)
target_link_libraries(tutorial9 PUBLIC GST_PB)

add_executable(borrowed_rtp borrowed_rtp_example.c)
target_link_libraries(borrowed_rtp PUBLIC GST_FLAGS)

add_executable(gst_thread gst_thread.cpp src/GstWrapper.cpp)
target_include_directories(gst_thread PUBLIC include)
target_link_libraries(gst_thread PUBLIC GST_FLAGS)

add_executable(gst_thread_san gst_thread.cpp src/GstWrapper.cpp)
target_include_directories(gst_thread_san PUBLIC include)
target_link_libraries(gst_thread_san PUBLIC GST_FLAGS)

target_compile_options(gst_thread_san PRIVATE -fsanitize=address)
target_link_options(gst_thread_san PRIVATE -fsanitize=address)

