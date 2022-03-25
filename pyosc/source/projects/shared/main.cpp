#include <iostream>
#include <thread>

#include "connector.h"
#include "text_gen.h"
#include <chrono>
#include <thread>

#define OUTPUT_BUFFER_SIZE 1024

int main() {



    try {
        Connector c("127.0.0.1", 9090, 9090);

        auto t1 = std::chrono::high_resolution_clock::now();

        for (int j = 0; j < 1; ++j) {
//            std::unique_ptr<char[]> buffer = std::make_unique<char[]>(OUTPUT_BUFFER_SIZE);
//            std::unique_ptr<osc::OutboundPacketStream> p = std::make_unique<osc::OutboundPacketStream>(buffer.get(), OUTPUT_BUFFER_SIZE);

//            *p
////                    << osc::BeginBundleImmediate
//                    << osc::BeginMessage("/test1")
//                    << true << 23 << (float) 3.1415 << "hello"
////                     << osc::EndMessage
//                    ;

//            auto msg = OscSendMessage(std::move(buffer), std::move(p));

            auto msg = OscSendMessage("/test1", 1024);
//            msg.add_string("bang");
            msg.add_string(TextGen::text10k());
            c.send(msg);
        }


        auto t2 = std::chrono::high_resolution_clock::now();

        std::cout << "f() took "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count()
                  << " milliseconds\n";


        int i = 0;
        std::string address = "/test1";
        while (true) {
            auto r = c.receive(address);
            if (!r.empty()) {
                i += r.size();
                std::cout << i << "\n";
                for (auto& msg: r) {
                    auto it = msg.ArgumentsBegin();
                    while (it != msg.ArgumentsEnd()) {
                        if (it->IsBool()) {
//                            std::cout << it->AsBool() << " ";
                        } else if (it->IsChar()) {
//                            std::cout << it->AsChar() << " ";
                        } else if (it->IsFloat()) {
//                            std::cout << it->AsFloat() << " ";
                        } else if (it->IsString()) {
                            auto str = it->AsString();
                            std::cout << str << " ";
                            std::cout << "\n" <<strlen(str) << "\n";
                        }
                        it++;
                    }

                    std::cout << "\n";
                }
            }
//            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        }

    } catch (std::runtime_error& e) {
        std::cerr << e.what() << "\n";
    }


    return 0;
}