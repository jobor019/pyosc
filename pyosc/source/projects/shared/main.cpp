#include <iostream>
#include <thread>

#include "juce_osc/juce_osc.h"
#include "connector.h"
#include "base_object.h"

int main() {
//    juce::OSCSender sender;
//    sender.connect("127.0.0.1", 8081);
//
//    for (int i = 0; i < 100000; ++i) {
//        sender.send(juce::OSCMessage("/hehe", i));
//    }



    PyOscReceiver recv;
    recv.connect(8080);

    while (true) {
        auto t1 = std::chrono::high_resolution_clock::now();


        auto m = recv.readMessage();
        while (m) {
//            std::cout << (*m).begin()->getInt32() << std::endl;
            m = recv.readMessage();
        }
        auto t2 = std::chrono::high_resolution_clock::now();

        std::cout << "f() took "
                  << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()
                  << " microseconds\n";
        std::this_thread::sleep_for(std::chrono::milliseconds (100));
    }


    return 0;
}