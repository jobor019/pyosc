
#ifndef PYOSC_JUCE_OSC_RECEIVER_H
#define PYOSC_JUCE_OSC_RECEIVER_H

#include "juce_osc_input_stream.h"


class PyOscReceiver {
public:
    PyOscReceiver() {
    }

    bool connect(int port_number) {
        if (!disconnect()) {    // TODO: Useless?
            return false;
        }
        socket.setOwned(new juce::DatagramSocket(false));
        if (!socket->bindToPort(port_number)) {
            return false;
        }
        // Normally: start thread here -> run
        return true;
    }

    bool disconnect() {
        if (socket != nullptr) {
            if (socket.willDeleteObject()) {
                socket->shutdown();
            }
            socket.reset();
        }
        return true;
    }


    std::optional<juce::OSCMessage> readMessage() {
        jassert(socket != nullptr); // TODO: Don't jassert
        auto ready = socket->waitUntilReady(true, 0);
        if (ready < 0) {
            std::cout << "error!\n";
            return std::nullopt; // TODO: Quit entire app - fatal error
        } else if (ready == 0) {
            return std::nullopt;  // TODO: timeout: inform about this
        } else {
            auto bytesRead = (size_t) socket->read(oscBuffer.getData(), bufferSize, false);
            if (bytesRead >= 4) {
                return handleBuffer(oscBuffer.getData(), bytesRead);
            }
        }
        return std::nullopt;
    }


private:
    juce::OptionalScopedPointer<juce::DatagramSocket> socket;// TODO: uniqueptr
    int bufferSize = 65535;
    juce::HeapBlock<char> oscBuffer{65535};

    std::optional<juce::OSCMessage> handleBuffer(const char* data, size_t dataSize) {
        OSCInputStream inStream(data, dataSize);

        try {
            auto content = inStream.readElementWithKnownSize(dataSize);

            if (content.isMessage()) {
                auto&& message = content.getMessage();
                return message;
            }
        } catch (const juce::OSCFormatError&) {
            // IGNORE
            std::cout << "format err\n";
        }
        return std::nullopt;

    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PyOscReceiver)
};

#endif //PYOSC_JUCE_OSC_RECEIVER_H
