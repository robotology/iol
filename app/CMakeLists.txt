# Copyright: (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.6)
find_package(ICUB)
list(APPEND CMAKE_MODULE_PATH ${ICUB_MODULE_PATH})

set(PROJECTNAME interactiveObjectsLearning)

file(GLOB scripts ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.template ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.ini)
file(GLOB rad     ${CMAKE_CURRENT_SOURCE_DIR}/RAD/*.*)
set(lua_fsm       ${CMAKE_CURRENT_SOURCE_DIR}/LUA/iol_funcs.lua
                  ${CMAKE_CURRENT_SOURCE_DIR}/LUA/iol_interact_fsm.lua
                  ${CMAKE_CURRENT_SOURCE_DIR}/LUA/iol_root_fsm.lua)
set(lua_bin       ${CMAKE_CURRENT_SOURCE_DIR}/LUA/iol_main.lua)

find_package(YARP)
find_package(ICUBcontrib)
list(APPEND CMAKE_MODULE_PATH ${YARP_MODULE_PATH} ${ICUBCONTRIB_MODULE_PATH})
include(YarpInstallationHelpers)
include(ICUBcontribHelpers)
icubcontrib_set_default_prefix()

yarp_install(FILES     ${scripts} DESTINATION ${ICUBCONTRIB_APPLICATIONS_TEMPLATES_INSTALL_DIR})
yarp_install(DIRECTORY RAD        DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME} PATTERN .svn EXCLUDE)
yarp_install(PROGRAMS  ${lua_fsm} DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME}/LUA)
yarp_install(PROGRAMS  ${lua_bin} DESTINATION bin)

