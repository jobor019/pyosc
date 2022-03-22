#include <iostream>

#include "juce_osc/juce_osc.h"

int main() {
    std::cout << "test" << std::endl;
    auto msg = juce::OSCMessage("/hehe");
    std::cout << msg.getAddressPattern().toString() << std::endl;
    return 0;
}