# Copyright: (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
# Authors: Carlo Ciliberto
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 3.5)
project(milClassifier)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

find_package(boostMIL QUIET)
if(boostMIL_FOUND)
    message(STATUS "boostMIL found")
endif()

find_package(SIFTGPU QUIET)
if(SIFTGPU_FOUND)
    message(STATUS "SIFGPU found")
endif()

if(boostMIL_FOUND AND SIFTGPU_FOUND)
    message(STATUS "building milClassifier")
    find_package(YARP REQUIRED)
    find_package(ICUB REQUIRED)
    list(APPEND CMAKE_MODULE_PATH ${ICUB_MODULE_PATH})

    find_package(OpenCV REQUIRED)
    if(OpenCV_VERSION_MAJOR GREATER 2)
       message(STATUS "detected Version(OpenCV) > 2.x.y => ${PROJECT_NAME} will not be built")
    else()
       find_package(ICUBcontrib REQUIRED)
       list(APPEND CMAKE_MODULE_PATH ${ICUBCONTRIB_MODULE_PATH})
       include(ICUBcontribHelpers)
       include(ICUBcontribOptions)
       icubcontrib_set_default_prefix()

       include_directories(${boostMIL_INCLUDE_DIRS})

       set(folder_source main.cpp SiftGPU_Extractor.cpp)
       set(folder_header SiftGPU_Extractor.h)
       source_group("Source Files" FILES ${folder_source})
       source_group("Header Files" FILES ${folder_header})

       include_directories(${PROJECT_SOURCE_DIR} ${SIFTGPU_INCLUDE_DIRS})
       add_executable(${PROJECT_NAME} ${folder_source} ${folder_header})
       target_link_libraries(${PROJECT_NAME} boostMIL ${OpenCV_LIBRARIES} ${YARP_LIBRARIES} ${SIFTGPU_LIBRARIES} ${CMAKE_DL_LIBS})
       install(TARGETS ${PROJECT_NAME} DESTINATION bin)

       add_subdirectory(app)
    endif()
endif()
