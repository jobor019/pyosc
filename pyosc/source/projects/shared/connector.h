#ifndef PYOSC_CONNECTOR_H
#define PYOSC_CONNECTOR_H


#include <iostream>

#include "juce_osc/juce_osc.h"
#include "juce_osc_receiver.h"
#include "oscpack_sender.h"
#include "oscpack_receiver.h"


class Connector : juce::OSCReceiver {

public:
    /**
     * @raises: std::runtime_error if fails to connect to port
     */
    Connector(const std::string& ip, int send_port, int recv_port) : sender(ip, send_port), receiver(recv_port) {
        receiver.run();
    }

    bool send(OscSendMessage& msg) {
        return false;
    }

    std::vector<osc::ReceivedMessage> receive(std::string& address) {
        std::lock_guard<std::mutex> myLock{recv_mutex};
        auto new_messages = receiver.receive();
        for (auto& msg: new_messages) {
            std::string msg_address = std::string(msg.AddressPattern());

            if (messages.find(msg_address) != messages.end()) {
                messages.at(msg_address).emplace_back(msg);
            } else {
                messages.insert(std::pair<std::string, std::vector<osc::ReceivedMessage>>(msg_address, {msg}));
            }
        }

        auto i = messages.find(address);
        if (i != messages.end()) {
            auto msgs = std::move(i->second);
            messages.erase(i);
            return msgs;
        }

        return {};
    }

    std::vector<osc::ReceivedMessage> terminate() {
        auto msgs = receiver.stop();
        terminated = true;
        return msgs;
    }


private:
    bool terminated = false;

    std::mutex send_mutex;
    std::mutex recv_mutex;

    OscPackSender sender;
    OscPackReceiver receiver;

    // TODO: Replacing this with a simple vector is probably faster for most use cases:
    //  fetching multiple messages will be slower but inserting much faster
    std::map<std::string, std::vector<osc::ReceivedMessage>> messages;

};


#endif //PYOSC_CONNECTOR_H