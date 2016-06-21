cmake_minimum_required(VERSION 2.8)
project( SoloUDP2 )

#Version control
set (SoloUDP_VERSION_MAJOR 1)
set (SoloUDP_VERSION_MINOR 0)
set (SoloUDP_VERSION_BUGFIX 0)

#configure header file to pass some of the CMake settings 
#to the source code
configure_file(  
    "${PROJECT_SOURCE_DIR}/SoloUDPConfig.h.in" 
    "${PROJECT_BINARY_DIR}/SoloUDPConfig.h" 
) 

find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

# add the binary tree to the search path for include files
# so that we will find header files for gstreamer
include_directories(
    include 
    ${GSTREAMER_INCLUDE_DIRS}
    ${PROJECT_BINARY_DIR})

add_executable( SoloUDP2 solo-stream2.c )
target_link_libraries( ${PROJECT_NAME} ${GSTREAMER_LIBRARIES})
