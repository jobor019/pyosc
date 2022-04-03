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

    /**
     * @raises: osc::MalformedMessageException if message has improper formatting
     */
    void send(OscSendMessage& msg) {
        std::lock_guard<std::mutex> send_lock{send_mutex};
        sender.send(msg);
    }

    std::vector<c74::min::atoms> receive(const std::string& address) {
        std::lock_guard<std::mutex> recv_lock{recv_mutex};
        auto new_messages = receiver.receive();
        for (auto& msg: new_messages) {
            if (messages.find(msg.get_address()) != messages.end()) {
                messages.at(msg.get_address()).emplace_back(msg.pop_message());
            } else {
                messages.insert(
                        std::pair<std::string, std::vector<c74::min::atoms> >(msg.get_address(), {msg.pop_message()}));
            }
        }

        auto i = messages.find(address);
        if (i != messages.end()) {
            std::vector<c74::min::atoms> msgs = std::move(i->second);
            messages.erase(i);
            return msgs;
        }

        return {};
    }

    // TODO: Should probably return anything left in receiver on terminating, but needs a viable strategy for
    //  handling addresses, just returning is insufficient (need to place them in `messages`)
    void terminate() {
//        std::lock_guard<std::mutex> recv_lock{recv_mutex};
//        auto msgs = receiver.stop();
        terminated = true;
//        return msgs;
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
    std::map<std::string, std::vector<c74::min::atoms> > messages;

};


#endif //PYOSC_CONNECTOR_H