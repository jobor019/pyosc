#include <iostream>
#include <thread>

#include "connector.h"
#include "text_gen.h"
#include "pyosc_objects.h"
#include "pyosc_manager.h"
#include "c74_min_atom.h"
//#include "source/min-api/include/c74_min_atom.h"
#include <chrono>
#include <thread>
#include <optional>

#define OUTPUT_BUFFER_SIZE 1024


// TODO: Mutexes in Server/Thread/Remote

int main() {

    c74::min::atoms atms;

    Connector c{"127.0.0.1", 7000, 7001};


    while (true) {

        auto msgs = c.receive("/s1/status");

        for (auto& msg: msgs) {
//            std::cout << msg.AddressPattern() <<
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));


    }


//    std::shared_ptr<Server> server1;
//
//    try {
//        std::string name = "s1";
//        server1 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::make_optional(
//                PortSpec{7000, 7001}));
//        std::cout << "[EXPECTED]: Created server1 (send:" << server1->get_send_port() << ", recv:"
//                  << server1->get_recv_port() << ")\n";
//    } catch (std::runtime_error& e) {
//        std::cout << "name already exists or port already exists\n";
//        return 1;
//    }
//
//
//    std::string init_msg;
//
//    server1->update_status();
//    server1->initialize(init_msg);
//    server1->update_status();
//
//    while (true) {
//        server1->update_status();
//        std::cout << static_cast<int>(server1->get_status()) << "\n";
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//
//
//        if (server1->get_status() == Status::ready) {
//            auto t1 = std::chrono::high_resolution_clock::now();
//            auto msg = OscSendMessage("/s1");
//            server1->send(msg);
//            auto ret = server1->receive("/s1");
//            while (ret.empty()) {
//                ret = server1->receive("/s1");
//                if (!ret.empty()) {
////                    std::cout << "MSG REC\n";
//                }
//            }
//            auto t2 = std::chrono::high_resolution_clock::now();
//
//            std::cout << "f() took "
//                      << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
//                      << " microseconds\n";
//
//        }
//
//    }




}