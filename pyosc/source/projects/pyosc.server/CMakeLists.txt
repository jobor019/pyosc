# Copyright 2018 The Min-DevKit Authors. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.

cmake_minimum_required(VERSION 3.15)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)


#############################################################
# MAX EXTERNAL
#############################################################


# Old-school CMake commands (~~target_~~include/link_directories/libraries) is needed for test compatibility
include_directories(
        "${C74_INCLUDES}"
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/"
        "${PROJECT_SOURCE_DIR}/../../oscpack/ip"
        "${PROJECT_SOURCE_DIR}/../../oscpack/osc"
)

link_directories(
        "${PROJECT_BINARY_DIR}/../../oscpack"
)

link_libraries(oscpack)


set(SOURCE_FILES
        ../shared/max_obj_base.h
        ../shared/atom_osc_parser.h
        ../src/pyosc_base.h
        ../src/connector.h
        ../src/juce_osc_receiver.h
        ../src/juce_osc_input_stream.h
        ../src/oscpack_receiver.h
        ../src/oscpack_sender.h
        ../src/text_gen.h
        ../src/pyosc_base.cpp
        ../src/pyosc_objects.h
        ../src/pyosc_manager.cpp
        ../src/pyosc_manager.h
        ../src/message_queue.h)


add_library(
        ${PROJECT_NAME}
        MODULE
        ${PROJECT_NAME}.cpp
        ${SOURCE_FILES}
)

#target_link_libraries(${PROJECT_NAME}
#        PUBLIC
#        oscpack)
#
#target_include_directories(${PROJECT_NAME} PUBLIC
#        "${PROJECT_BINARY_DIR}"
#        "${PROJECT_SOURCE_DIR}/"
#        "${PROJECT_SOURCE_DIR}/../../oscpack/ip"
#        "${PROJECT_SOURCE_DIR}/../../oscpack/osc"
#        )


#target_include_directories(k
#		${PROJECT_NAME}
#		PUBLIC
#		../shared)
#
#target_link_directories(
#		${PROJECT_NAME}
#		PUBLIC
#		../shared)

#target_link_libraries(${PROJECT_NAME}
#		PRIVATE
#		pyosc)


include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)


#############################################################
# UNIT TEST
#############################################################

include(${C74_MIN_API_DIR}/test/min-object-unittest.cmake)
