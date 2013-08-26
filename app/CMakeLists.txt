# Copyright: (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.6)
find_package(ICUB)
set(CMAKE_MODULE_PATH ${ICUB_MODULE_PATH} ${CMAKE_MODULE_PATH})
include(iCubHelpers)

set(PROJECTNAME interactiveObjectsLearning)
icub_app(${PROJECTNAME})

file(GLOB scripts ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.template ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.ini)
file(GLOB lua     ${CMAKE_CURRENT_SOURCE_DIR}/LUA/*.lua)
file(GLOB rad     ${CMAKE_CURRENT_SOURCE_DIR}/RAD/*.*)

set(ICUB_APPLICATIONS_PREFIX "$ENV{ICUB_ROOT}" CACHE PATH "Application path prefix")
icub_app_install(${PROJECTNAME} 
                 FILES ${scripts}
                 DESTINATION scripts)

icub_app_install(${PROJECTNAME} 
                 FILES ${lua}
                 DESTINATION LUA)

icub_app_install(${PROJECTNAME} 
                 FILES ${rad}
                 DESTINATION RAD)

### create a single target that installs all applications at once
icub_app_all()

find_package(YARP)
find_package(ICUBcontrib)
list(APPEND CMAKE_MODULE_PATH ${YARP_MODULE_PATH} ${ICUBCONTRIB_MODULE_PATH})
include(YarpInstallationHelpers)
include(ICUBcontribHelpers)
set_contrib_default_prefix()

yarp_install(DIRECTORY RAD DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME})
yarp_install(DIRECTORY LUA DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME})
yarp_install(FILES ${scripts} DESTINATION ${ICUBCONTRIB_APPLICATIONS_TEMPLATES_INSTALL_DIR})
