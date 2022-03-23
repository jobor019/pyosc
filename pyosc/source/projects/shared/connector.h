#ifndef PYOSC_CONNECTOR_H
#define PYOSC_CONNECTOR_H


#include <iostream>

#include "juce_osc/juce_osc.h"
#include "osc_receiver.h"



class Connector : juce::OSCReceiver {

public:
    Connector(const std::string& ip, int send_port, int recv_port) : send_port(send_port), recv_port(recv_port) {
        if (!sender.connect(ip, send_port)) {
            throw std::runtime_error("(send) could not connect to udp port " + std::to_string(send_port));
        }
        if (!receiver.connect(send_port)) {
            throw std::runtime_error("(receive) could not connect to udp port " + std::to_string(recv_port) +
                                     " with ip " + ip);
        }
    }

    bool send(juce::OSCMessage& msg) {
        return false;
    }

    std::vector<juce::OSCMessage> receive(juce::OSCAddressPattern& address) {
        return {};
    }

    void terminate() {
        terminated = true;
    }


private:
    int send_port;
    int recv_port;
    bool terminated = false;

    std::mutex send_mutex;
    std::mutex recv_mutex;

    juce::OSCSender sender;
    PyOscReceiver receiver;
};


#endif //PYOSC_CONNECTOR_H