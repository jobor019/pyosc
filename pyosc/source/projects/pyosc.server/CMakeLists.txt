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
)


set( SOURCE_FILES
	${PROJECT_NAME}.cpp
		../shared/max_obj_base.h
		../shared/atom_osc_parser.h)


add_library(
	${PROJECT_NAME}
	MODULE
	${SOURCE_FILES}
)

#target_include_directories(k
#		${PROJECT_NAME}
#		PUBLIC
#		../shared)
#
#target_link_directories(
#		${PROJECT_NAME}
#		PUBLIC
#		../shared)

target_link_libraries(${PROJECT_NAME}
		PRIVATE
		pyosc)





include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)


#############################################################
# UNIT TEST
#############################################################

include(${C74_MIN_API_DIR}/test/min-object-unittest.cmake)
