//
// Created by Joakim Borg on 25/03/2022.
//

#ifndef PYOSC_OSC_SEND_MESSAGE_H
#define PYOSC_OSC_SEND_MESSAGE_H

#include <iostream>
#include "OscOutboundPacketStream.h"
#include "OscReceivedElements.h"

class OscSendMessage {
public:

    explicit OscSendMessage(std::string address, int buffer_size = 1024) {
        buffer = std::make_unique<char[]>(buffer_size);
        stream = std::make_unique<osc::OutboundPacketStream>(buffer.get(), buffer_size);

        *stream << osc::BeginMessage(address.c_str());

    }

    OscSendMessage(std::unique_ptr<char[]> buffer, std::unique_ptr<osc::OutboundPacketStream> stream) :
            buffer(std::move(buffer))
            , stream(std::move(stream)) {}

    /**
     * @raises: osc::OutOfBufferMemoryException if buffer is full
     */
    void add_bool(bool arg) {
        *stream << arg;
    }

    /**
     * @raises: osc::OutOfBufferMemoryException if buffer is full
     */
    void add_int(int arg) {
        *stream << arg;
    }

    /**
     * @raises: osc::OutOfBufferMemoryException if buffer is full
     */
    void add_long(long arg) {
        *stream << arg;
    }

    /**
     * @raises: osc::OutOfBufferMemoryException if buffer is full
     */
    void add_float(float arg) {
        *stream << arg;
    }

    /**
     * @raises: osc::OutOfBufferMemoryException if buffer is full
     */
    void add_double(double arg) {
        *stream << arg;
    }


    /**
     * @raises: osc::OutOfBufferMemoryException if buffer is full
     */
    void add_char(char arg) {
        *stream << arg;
    }

    /**
     * @raises: osc::OutOfBufferMemoryException if buffer is full
     */
    void add_string(std::string arg) {
        *stream << arg.c_str();
    }


    [[nodiscard]] osc::OutboundPacketStream* get_sendable_stream() const {
        if (stream->IsMessageInProgress()) {
            std::cout << "ending message\n";
            std::cout << (stream.get()) << "\n";
            *stream << osc::EndMessage;
        }
        // While it's technically possibly to pass a bundle to this class at the moment, it shouldn't check for this
        // And if it should, it should iteratively check for `IsMessageInProgress()` until false rather than single check
        //        if (stream->IsBundleInProgress()) {
        //            *stream << osc::EndBundle;
        //        }
        return stream.get();
    }

private:
    std::unique_ptr<char[]> buffer;
    std::unique_ptr<osc::OutboundPacketStream> stream;

};

#endif //PYOSC_OSC_SEND_MESSAGE_H
