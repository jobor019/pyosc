# Copyright 2018 The Min-DevKit Authors. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.

cmake_minimum_required(VERSION 3.15)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)


#############################################################
# MAX EXTERNAL
#############################################################


include_directories(
        "${C74_INCLUDES}"
        "${PROJECT_SOURCE_DIR}/../../oscpack/ip"
        "${PROJECT_SOURCE_DIR}/../../oscpack/osc"
)

link_directories(
        "${PROJECT_BINARY_DIR}/../../oscpack"
)

link_libraries(oscpack)


set( SOURCE_FILES
        ${PROJECT_NAME}.cpp
        ../src/atom_osc_parser.h
        ../src/connector.h
        ../src/message_queue.h
        ../src/osc_send_message.h
        ../src/oscpack_receiver.h
        ../src/oscpack_sender.h
        ../src/pyosc_base.cpp
        ../src/pyosc_manager.cpp
        ../src/pyosc_objects.h
        ../src/max_base.h
        )


add_library(
        ${PROJECT_NAME}
        MODULE
        ${SOURCE_FILES}
)



include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)


#############################################################
# UNIT TEST
#############################################################


#include(${C74_MIN_API_DIR}/test/min-object-unittest.cmake)
