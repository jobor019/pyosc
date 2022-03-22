cmake_minimum_required(VERSION 3.15)


add_library(
        pyosc
        SHARED
        agent_object.h
        base_pyosc_object.h
        connector.h
        pyosc_manager.h
        pyosc_max_base.h
)

target_link_libraries(pyosc
        PRIVATE
        juce::juce_core
        juce::juce_osc)


add_executable(standalone
        main.cpp
        agent_object.h
        base_pyosc_object.h
        connector.h
        pyosc_manager.h
        pyosc_max_base.h)

target_link_libraries(standalone
        PRIVATE
        juce::juce_core
        juce::juce_osc)