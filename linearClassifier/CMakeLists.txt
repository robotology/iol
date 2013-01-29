# Copyright: 2010 RobotCub Consortium
# Author: Sean Ryan Fanello, Carlo Ciliberto
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

include(FindLibSVMLin.cmake)
include(FindLibSVM.cmake)



IF(LIBSVM_FOUND)
IF(LIBSVMLIN_FOUND)
	find_package(YARP REQUIRED)
	find_package(ICUB REQUIRED)
	cmake_minimum_required(VERSION 2.6)
	set(projectname linearClassifierModule)
	project(${projectname})
	file(GLOB source src/*.cpp)
	file(GLOB header include/*.h)
	source_group("Source Files" FILES ${source})
	source_group("Header Files" FILES ${header})
	include_directories(${LIBSVM_INCLUDE_DIR})
	include_directories(${LIBSVMLIN_INCLUDE_DIR})
	include_directories(${YARP_INCLUDE_DIRS})
	include_directories(${ICUB_INCLUDE_DIRS})
	include_directories(${PROJECT_SOURCE_DIR}/include)

	add_executable(${projectname} ${source} ${header} )
	target_link_libraries(${projectname} ${YARP_LIBRARIES} ${LIBSVMLIN_LIBRARIES} ${LIBSVM_LIBRARIES})
	install(TARGETS ${projectname} DESTINATION ${ICUB_DIR}/bin)
ENDIF()
ENDIF()
