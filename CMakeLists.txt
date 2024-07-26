cmake_minimum_required(VERSION 3.27)
project(gstreamer_box)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig)
find_package(Threads REQUIRED)

# May need to set environment variable PKG_CONFIG_PATH
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

add_library(GST_FLAGS INTERFACE)
target_compile_options(GST_FLAGS INTERFACE ${GSTREAMER_CFLAGS} ${GSTREAMER_CFLAGS_OTHER})
target_link_options(GST_FLAGS INTERFACE ${GSTREAMER_LDFLAGS})
target_link_libraries(GST_FLAGS INTERFACE ${GSTREAMER_LIBRARIES} Threads::Threads)

add_executable(hola_gstreamer hola_gstreamer.cpp)
target_link_libraries(hola_gstreamer PUBLIC GST_FLAGS)

add_executable(simple_rtp simple_rtp.cpp)
target_link_libraries(simple_rtp PUBLIC GST_FLAGS)


