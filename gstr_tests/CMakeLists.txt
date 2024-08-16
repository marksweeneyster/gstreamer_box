pkg_check_modules(GSTREAMER_RTSP_SRVR REQUIRED gstreamer-rtsp-server-1.0)
# PB Utils includes all the base gstreamer settings
add_library(GST_RTSP_SRVR INTERFACE)
target_include_directories(GST_RTSP_SRVR INTERFACE ${GSTREAMER_RTSP_SRVR_INCLUDE_DIRS})
target_link_directories(GST_RTSP_SRVR INTERFACE ${GSTREAMER_RTSP_SRVR_LIBRARY_DIRS})
target_link_libraries(GST_RTSP_SRVR INTERFACE ${GSTREAMER_RTSP_SRVR_LIBRARIES})

add_executable(test_multicast test_multicast.c)
target_link_libraries(test_multicast PUBLIC GST_RTSP_SRVR)

add_executable(test_appsrc test_appsrc.c)
target_link_libraries(test_appsrc PUBLIC GST_RTSP_SRVR)
