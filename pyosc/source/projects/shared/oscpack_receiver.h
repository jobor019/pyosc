
#ifndef PYOSC_OSCPACK_RECEIVER_H
#define PYOSC_OSCPACK_RECEIVER_H

#include <iostream>
#include <vector>
#include <thread>

#include "OscPacketListener.h"
#include "UdpSocket.h"


class OscPackReceiver : public osc::OscPacketListener {

public:
    explicit OscPackReceiver(int port) : socket(IpEndpointName(IpEndpointName::ANY_ADDRESS, port), this) {}

    ~OscPackReceiver() override {
        socket.Break();
        thread.join();
    }

    /**
     * @raises: std::runtime_error if fails to connect to port
     */
    void run() {
        thread = std::thread([this]() { this->start_socket(); });
    }

    std::vector<osc::ReceivedMessage> stop() {
        stop_socket();
        return receive();
    }

    std::vector<osc::ReceivedMessage> receive() {

        std::lock_guard<std::mutex> myLock{mutex};
        std::vector<osc::ReceivedMessage> vec;
        std::swap(received_messages, vec);
        return vec;

    }

protected:

    void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint) override {
        (void) remoteEndpoint; // suppress unused parameter warning

        std::lock_guard<std::mutex> myLock{mutex};
        received_messages.emplace_back(m);

    }

private:
    UdpListeningReceiveSocket socket;

    std::vector<osc::ReceivedMessage> received_messages;
    std::mutex mutex; // Should never be locked for more than a microsecond or two

    std::thread thread;

    void start_socket() {
        socket.Run();
    }

    void stop_socket() {
        socket.Break();
    }
};

#endif //PYOSC_OSCPACK_RECEIVER_H
