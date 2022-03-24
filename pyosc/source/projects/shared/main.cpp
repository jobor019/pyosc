#include <iostream>
#include <thread>

#include "connector.h"
#include <chrono>
#include <thread>



int main() {

    int i = 0;

    try {
        Connector c("127.0.0.1", 9090, 9091);


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
//                            std::cout << it->AsString() << " ";
                        }
                        it++;
                    }

//                    std::cout << "\n";
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        }

    } catch (std::runtime_error& e) {
        std::cerr << e.what() << "\n";
    }


    return 0;
}