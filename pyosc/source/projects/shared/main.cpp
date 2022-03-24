#include <iostream>
#include <thread>

#include "juce_osc/juce_osc.h"
#include "connector.h"
#include "base_object.h"
#include "text_gen.h"

#include <OscOutboundPacketStream.h>
#include <UdpSocket.h>

#define ADDRESS "127.0.0.1"
#define PORT 7000

#define OUTPUT_BUFFER_SIZE 1024

int main() {

    UdpTransmitSocket transmitSocket(IpEndpointName(ADDRESS, PORT));

    char buffer[OUTPUT_BUFFER_SIZE];
    osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);

    int dropped = 0;

    int i = 0;
    while (i < 10000) {
        try {
            p
//    << osc::BeginBundleImmediate
                    << osc::BeginMessage("/test1")
                    << true << -1 << (float) 3.1415 << "hello" << osc::EndMessage;
//        << osc::BeginMessage( "/test2" )
//            << true << 24 << (float)10.8 << "world" << osc::EndMessage
//        << osc::EndBundle;
        } catch (osc::OutOfBufferMemoryException& e) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            dropped++;
        }
        transmitSocket.Send(p.Data(), p.Size());
        i++;




        std::cout << i << " " << dropped << "\n";
    }
    std::cout << "dropped: " << dropped << "\n";

    juce::OSCSender sender;
    sender.connect("127.0.0.1", 7000);

//    auto text = TextGen::text10k();

//    std::cout << juce::OSCArgument(text).getString().length() << "\n";
//    auto status = sender.send(juce::OSCMessage("/test1", juce::OSCArgument(juce::String(text))));
//    std::cout << status << "\n";

//    auto str = std::string("hehe");
//    sender.send(juce::OSCMessage("/t", juce::OSCArgument(str)));

    for (int i = 0; i < 100000; ++i) {
        sender.send(juce::OSCMessage("/test1", juce::OSCArgument("bang")));
    }






//    PyOscReceiver recv;
//    recv.connect(8080);
//
//    while (true) {
//        auto t1 = std::chrono::high_resolution_clock::now();
//
//
//        auto m = recv.readMessage();
//        while (m) {
////            std::cout << (*m).begin()->getInt32() << std::endl;
//            m = recv.readMessage();
//        }
//        auto t2 = std::chrono::high_resolution_clock::now();
//
//        std::cout << "f() took "
//                  << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()
//                  << " microseconds\n";
//        std::this_thread::sleep_for(std::chrono::milliseconds (100));
//    }


    return 0;
}