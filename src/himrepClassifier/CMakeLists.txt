# Copyright: 2010 RobotCub Consortium
# Author: Sean Ryan Fanello
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 3.5)
set(projectname himrepClassifier)

find_package(ICUBcontrib REQUIRED)
find_package(YARP 3.1.100 REQUIRED COMPONENTS OS sig cv)
find_package(ICUB REQUIRED)
find_package(OpenCV REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${ICUB_MODULE_PATH})
list(APPEND CMAKE_MODULE_PATH ${ICUBCONTRIB_MODULE_PATH})
include(ICUBcontribHelpers)
include(ICUBcontribOptions)
icubcontrib_set_default_prefix()

project(${projectname})
file(GLOB source *.cpp)
file(GLOB header *.h)
source_group("Source Files" FILES ${source})
source_group("Header Files" FILES ${header})
include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(${projectname} ${source} ${header})
target_link_libraries(${projectname} ${YARP_LIBRARIES} ${OpenCV_LIBRARIES})
install(TARGETS ${projectname} DESTINATION bin)
