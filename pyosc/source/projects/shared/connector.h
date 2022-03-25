#ifndef PYOSC_CONNECTOR_H
#define PYOSC_CONNECTOR_H


#include <iostream>
#include <map>

#include "oscpack_sender.h"
#include "oscpack_receiver.h"
#include "osc_send_message.h"


class Connector {

public:
    /**
     * @raises: std::runtime_error if fails to connect to port
     */
    Connector(const std::string& ip, int send_port, int recv_port) : send_port(send_port)
                                                                     , recv_port(recv_port)
                                                                     , sender(ip, send_port)
                                                                     , receiver(recv_port) {
        receiver.run();
    }

    void send(OscSendMessage& msg) {
        std::lock_guard<std::mutex> send_lock{send_mutex};
        sender.send(msg);
    }

    std::vector<osc::ReceivedMessage> receive(std::string& address) {
        std::lock_guard<std::mutex> recv_lock{recv_mutex};
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
        std::lock_guard<std::mutex> recv_lock{recv_mutex};
        auto msgs = receiver.stop();
        terminated = true;
        return msgs;
    }

    [[nodiscard]] bool is_terminated() const {
        return terminated;
    }

    [[nodiscard]] int get_send_port() const {
        return send_port;
    }

    [[nodiscard]] int get_recv_port() const {
        return recv_port;
    }


private:
    bool terminated = false;

    std::mutex send_mutex;
    std::mutex recv_mutex;

    const int send_port;
    const int recv_port;

    OscPackSender sender;
    OscPackReceiver receiver;

    // TODO: Replacing this with a simple vector is probably faster for most use cases:
    //  fetching multiple messages will be slower but inserting much faster
    std::map<std::string, std::vector<osc::ReceivedMessage>> messages;

};


#endif //PYOSC_CONNECTOR_H