# Copyright: (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.6)
find_package(ICUB)
list(APPEND CMAKE_MODULE_PATH ${ICUB_MODULE_PATH})
include(iCubHelpers)

set(PROJECTNAME interactiveObjectsLearning)

file(GLOB scripts ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.template ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.ini)
file(GLOB lua     ${CMAKE_CURRENT_SOURCE_DIR}/LUA/*.lua)
file(GLOB rad     ${CMAKE_CURRENT_SOURCE_DIR}/RAD/*.*)

find_package(YARP)
find_package(ICUBcontrib)
list(APPEND CMAKE_MODULE_PATH ${YARP_MODULE_PATH} ${ICUBCONTRIB_MODULE_PATH})
include(YarpInstallationHelpers)
include(ICUBcontribHelpers)
set_contrib_default_prefix()

yarp_install(DIRECTORY RAD        DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME} PATTERN .svn EXCLUDE)
yarp_install(PROGRAMS  ${lua}     DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME}/LUA)
yarp_install(FILES     ${scripts} DESTINATION ${ICUBCONTRIB_APPLICATIONS_TEMPLATES_INSTALL_DIR})

