//
// Created by Joakim Borg on 24/03/2022.
//

#ifndef PYOSC_OSCPACK_SENDER_H
#define PYOSC_OSCPACK_SENDER_H

#include <iostream>
#include "UdpSocket.h"
#include "OscOutboundPacketStream.h"

class OscSendMessage {
    OscSendMessage(std::unique_ptr<char[]> buffer, std::unique_ptr<osc::OutboundPacketStream> stream) : buffer(
            std::move(buffer)), stream(std::move(stream)) {}

    std::unique_ptr<char[]> buffer;
    std::unique_ptr<osc::OutboundPacketStream> stream;

public:
    [[nodiscard]] char* getBuffer() const {
        return buffer.get();
    }

    [[nodiscard]] osc::OutboundPacketStream* getStream() const {
        return stream.get();
    }
};

class OscPackSender {
    //
public:
    OscPackSender(const std::string& ip, int port) : socket(IpEndpointName(ip.c_str(), port)) {}


    void send(OscSendMessage& message) {
        auto* stream = message.getStream();
        socket.Send(stream->Data(), stream->Size());
    }


private:
    UdpTransmitSocket socket;

};

#endif //PYOSC_OSCPACK_SENDER_H
