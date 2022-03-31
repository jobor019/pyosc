
#ifndef PYOSC_OSCPACK_RECEIVER_H
#define PYOSC_OSCPACK_RECEIVER_H

#include <iostream>
#include <vector>
#include <thread>

#include "OscPacketListener.h"
#include "UdpSocket.h"


class OscPackReceiver : public osc::OscPacketListener {

public:
    explicit OscPackReceiver(int port) : socket(std::make_unique<UdpListeningReceiveSocket>(IpEndpointName(IpEndpointName::ANY_ADDRESS, port), this)) {}

    // TODO: Rule of 3/5: Implement/delete move/copy contructors/assignment operators, etc.
    ~OscPackReceiver() override {
        // Break loop and interrupt any ongoing socket.select command
        socket->AsynchronousBreak();
        thread.join();
    }

    /**
     * @raises: std::runtime_error if fails to connect to port
     */
    void run() {
        thread = std::thread([this]() { this->socket->Run(); });
    }

    std::vector<osc::ReceivedMessage> stop() {
        socket->AsynchronousBreak();
        return receive();
    }

    std::vector<osc::ReceivedMessage> receive() {

        std::lock_guard<std::mutex> receive_lock{mutex};
        std::vector<osc::ReceivedMessage> vec;
        std::swap(received_messages, vec);
        return vec;

    }

protected:

    void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint) override {
        (void) remoteEndpoint; // suppress unused parameter warning

        std::lock_guard<std::mutex> receive_lock{mutex};
        received_messages.emplace_back(m);

    }

private:
    std::unique_ptr<UdpListeningReceiveSocket> socket;

    std::vector<osc::ReceivedMessage> received_messages;
    std::mutex mutex; // Should never be locked for more than a microsecond or two

    std::thread thread;

};

#endif //PYOSC_OSCPACK_RECEIVER_H
