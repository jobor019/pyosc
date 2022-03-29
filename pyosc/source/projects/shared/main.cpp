#include <iostream>
#include <thread>

#include "connector.h"
#include "text_gen.h"
#include "pyosc_manager.h"
#include <chrono>
#include <thread>

#define OUTPUT_BUFFER_SIZE 1024

int main() {
    auto r = std::make_unique<OscPackReceiver>(8887);
    r->run();
    r->stop();
    r.reset();


    auto c = std::make_unique<Connector>("127.0.0.1", 8888, 8889);
    c->terminate();
    c = nullptr;


    auto socket = std::make_unique<UdpSocket>();
    socket->Bind(IpEndpointName(IpEndpointName::ANY_ADDRESS, 7070));


    auto& manager = PyOscManager::get_instance();
    std::string a1 = "a1";
    std::string a2 = "a2";
    std::string a3 = "a11";


    auto res = manager.add_object(a1);
    std::cout << res << "\n";

    res = manager.add_object(a1);
    std::cout << res << "\n";

    res = manager.add_object(a3);
    std::cout << res << "\n";


    // Initializing object without default port should work
    auto c1 = manager.new_connector(a1);
    std::cout << "send: " << c1->get_send_port() << " recv: " << c1->get_recv_port() << "\n";

    // Initializing object again should throw std::runtime_error
    try {
        auto c2 = manager.new_connector(a1);
    } catch (std::runtime_error& e) {
        std::cout << "[EXPECTED ERROR]: " << e.what() << "\n";
    }

    // Object not previously added
    auto c2 = manager.new_connector(a2);
    std::cout << "send: " << c2->get_send_port() << " recv: " << c2->get_recv_port() << "\n";


    // Adding a2 now should return 0
    res = manager.add_object(a2);
    std::cout << "Add a2 [0]: " << res << "\n";

    // Removing a2
    res = manager.remove_object(a2);
    std::cout << "Remove a2 [1]: " << res << "\n";


    // Removing a2 that doesn't exist
    res = manager.remove_object(a2);
    std::cout << "Remove a2 [0]: " << res << "\n";

    // Adding a2 should be same ports as above
    auto c3 = manager.new_connector(a2);
    std::cout << "Add a2 [same ports as above] send: " << c2->get_send_port() << " recv: " << c2->get_recv_port()
              << "\n";
    res = manager.remove_object(a2);

    // Manual pathspec (not taken)
    auto c4 = manager.new_connector(a2, PortSpec{"127.0.0.1", 8080, 8081});
    std::cout << "manual pathspec [8080, 8081] send: " << c4->get_send_port() << " recv: " << c4->get_recv_port()
              << "\n";

    // Manual pathspec (taken)
    try {
        auto c4 = manager.new_connector(a2, PortSpec{"127.0.0.1", 8080, 8081});
    } catch (std::runtime_error& e) {
        std::cout << "[EXPECTED ERROR]: " << e.what() << "\n";
    }


    // Manual pathspec (ports outside available/allowed range, but working ports)
    res = manager.remove_object(a2);
    auto c5 = manager.new_connector(a2, PortSpec{"127.0.0.1", 1234, 1235});
    std::cout << "manual pathspec [1234, 1235] send: " << c5->get_send_port() << " recv: " << c5->get_recv_port()
              << "\n";

    // Manual pathspec (previously taken but since object was deleted, should be available again)
    res = manager.remove_object(a2);
    c4.reset();
    auto c6 = manager.new_connector(a2, PortSpec{"127.0.0.1", 8080, 8081});
    std::cout << "manual pathspec after deletion [8080, 8081] send: " << c6->get_send_port() << " recv: " << c6->get_recv_port()
              << "\n";

    std::cout << "looping...\n";

    while (true) {

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    return 0;
}