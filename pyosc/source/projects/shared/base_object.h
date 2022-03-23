#ifndef PYOSC_BASE_OBJECT_H
#define PYOSC_BASE_OBJECT_H

#include <iostream>
#include <utility>
#include "juce_osc/juce_osc.h"

enum class Status {
    offline = 0
};

class BaseObject {
public:
    BaseObject(std::string& name, juce::OSCAddressPattern address) : name(name), address(std::move(address)) {}

    virtual bool initialize() {
        return false;
    }

    virtual void send(juce::OSCMessage& msg) {
    }

    virtual std::vector<juce::OSCMessage> receive() {
        return {};
    }


    bool terminate() {
        return false;
    }

    Status status() {
        return Status::offline;
    }


private:
    std::string name;
    juce::OSCAddressPattern address;

    std::vector<juce::OSCMessage> send_queue;

};


class ServerObject : public BaseObject {
public:

    explicit ServerObject(std::string& name) : BaseObject(name, juce::OSCAddressPattern("/" + name)) {}

private:

};

#endif //PYOSC_BASE_OBJECT_H