cmake_minimum_required(VERSION 3.15)


set(SOURCE_FILES
        pyosc_base.h
        connector.h
        pyosc_manager.h
        pyosc_max_base.h
        juce_osc_receiver.h
        juce_osc_input_stream.h
        oscpack_receiver.h
        oscpack_sender.h
        text_gen.h
        pyosc_base.cpp pyosc_objects.h)


#add_library(
#        pyosc
#        SHARED
#        ${SOURCE_FILES}
#)

#target_link_libraries(pyosc
#        PUBLIC
#        juce::juce_core
#        juce::juce_osc)


add_executable(standalone
        main.cpp
        ${SOURCE_FILES}
        )


target_link_libraries(standalone
        PUBLIC
        oscpack)

target_include_directories(standalone PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/"
        "${PROJECT_SOURCE_DIR}/source/oscpack/ip"
        "${PROJECT_SOURCE_DIR}/source/oscpack/osc"
        )


message("${PROJECT_SOURCE_DIR}/source/oscpack")
