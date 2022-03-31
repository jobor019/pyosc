//
// Created by Joakim Borg on 24/03/2022.
//

#ifndef PYOSC_OSCPACK_SENDER_H
#define PYOSC_OSCPACK_SENDER_H

#include <iostream>
#include "UdpSocket.h"
#include "OscOutboundPacketStream.h"
#include "osc_send_message.h"

class OscPackSender {
    //
public:
    OscPackSender(const std::string ip, int port) : socket(IpEndpointName(ip.c_str(), port)) {}


    /**
     * @raises: osc::MalformedMessageException if message has improper formatting
     */
    void send(OscSendMessage& message) {
        auto* stream = message.get_sendable_stream();
        socket.Send(stream->Data(), stream->Size());
    }


private:
    UdpTransmitSocket socket;

};

#endif //PYOSC_OSCPACK_SENDER_H
