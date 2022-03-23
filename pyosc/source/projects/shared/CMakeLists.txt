cmake_minimum_required(VERSION 3.15)

set(SOURCE_FILES
        agent_object.h
        base_object.h
        connector.h
        pyosc_manager.h
        pyosc_max_base.h
        osc_receiver.h
        juce_osc_input_stream.h
        remote_object.h
        )


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
        juce::juce_core
        juce::juce_osc)