#include <iostream>
#include <thread>

#include "connector.h"
#include "text_gen.h"
#include "pyosc_objects.h"
#include "pyosc_manager.h"
#include <chrono>
#include <thread>
#include <optional>

#define OUTPUT_BUFFER_SIZE 1024

int main() {


    std::shared_ptr<Server> server1;
    std::shared_ptr<Server> server2;
    std::shared_ptr<Server> server3;
    std::shared_ptr<Server> server4;
    std::shared_ptr<Server> server5;
    std::shared_ptr<Server> server6;

    try {
        std::string name = "server1";
        server1 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::nullopt);
        std::cout << "[EXPECTED]: Created server1 (send:" << server1->get_send_port() << ", recv:" << server1->get_recv_port() << ")\n";
    } catch (std::runtime_error& e) {
        std::cout << "name already exists or port already exists\n";
        return 1;
    }

    // Should work
    try {
        std::string name = "server2";
        server2 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::nullopt);
        std::cout << "[EXPECTED]: Created server2 (send:" << server2->get_send_port() << ", recv:" << server2->get_recv_port() << ")\n";
    } catch (std::runtime_error& e) {
        std::cout << "name already exists or port already exists\n";
        return 1;
    }

    // Expected Fail: duplicate name
    try {
        std::string name = "server1";
        server3 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::nullopt);
        std::cout << "Created server1 (send:" << server3->get_send_port() << ", recv:" << server3->get_recv_port() << ")\n";
    } catch (std::runtime_error& e) {
        std::cout << "[EXPECTED]: " << e.what() << "\n";
    }

    // Expected Fail: used send port
    try {
        std::string name = "server3";
        server4 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::make_optional(PortSpec{7000, 8000}));
        std::cout << "Created server3 (send:" << server4->get_send_port() << ", recv:" << server4->get_recv_port() << ")\n";
    } catch (std::runtime_error& e) {
        std::cout << "[EXPECTED]: " << e.what() << "\n";
    }

    // Expected Fail: used recv port
    try {
        std::string name = "server3";
        server5 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::make_optional(PortSpec{8000, 7000}));
        std::cout << "Created server3 (send:" << server5->get_send_port() << ", recv:" << server5->get_recv_port() << ")\n";
    } catch (std::runtime_error& e) {
        std::cout << "[EXPECTED]: " << e.what() << "\n";
    }

    // Should work: name and ports deallocated
    PyOscManager::get_instance().remove_object(server1->get_name());
//    PyOscManager::get_instance().reallocate_port(server1->get_send_port());
//    PyOscManager::get_instance().reallocate_port(server1->get_recv_port());
    server1.reset();
    try {
        std::string name = "server1";
        server6 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::make_optional(PortSpec{7000, 7001}));
        std::cout << "[EXPECTED]: Created server1 (send:" << server6->get_send_port() << ", recv:" << server6->get_recv_port() << ")\n";
    } catch (std::runtime_error& e) {
        std::cout << e.what() << "\n";
        return 1;
    }

    if (PyOscManager::get_instance().num_objects() == 2) {
        std::cout << "[EXPECTED]: " <<  PyOscManager::get_instance().num_objects() << " objects allocated on server\n";
    } else {
        std::cout << "Wrong number of allocations: " << PyOscManager::get_instance().num_objects() << "\n";
        return 1;
    }

    std::cout << "================================ Thread ================================\n";

    std::shared_ptr<Thread> thread1;
    std::shared_ptr<Thread> thread2;
    std::shared_ptr<Thread> thread3;

    std::shared_ptr<Server> server7;

    std::string server_name = "not_yet_created_server";
    // Creating object without existing server connection
    try {
        std::string name = "thread1";
        thread1 = PyOscManager::get_instance().new_thread(name, "/status", "terminate_agent $NAME", "127.0.0.1"
                                                          , std::nullopt, server_name);
        std::cout << "[EXPECTED]: Created " << name << " (send:" << thread1->get_send_port() << ", recv:" << thread1->get_recv_port() << ")\n";
    } catch (std::runtime_error& e) {
        std::cout << e.what() << "\n";
        return 1;
    }

    // Status: parent missing
    thread1->update_status();
    if (thread1->get_status() == Status::parent_obj_missing) {
        std::cout << "[EXPECTED]: parent missing\n";
    } else {
        std::cout << "Invalid status: " << static_cast<int>(thread1->get_status()) << "\n";
        return 1;
    }

    // Creating parent server
    server7 = PyOscManager::get_instance().new_server(server_name, "/status", "terminate", "127.0.0.1", std::nullopt);


    server7->update_status();
    assert(server7->get_status() == Status::uninitialized);
    server7->initialize(OscSendMessage("/null"));
    server7->update_status();
    assert(server7->get_status() == Status::initializing);


    // Linking thread1 to existing server
    auto obj = PyOscManager::get_instance().get_object(thread1->get_parent_name());
    thread1->set_parent(obj);

    thread1->update_status();
    assert(thread1->get_status() == Status::parent_obj_not_ready);

    // should return false since server isn't ready
    assert(!thread1->initialize(OscSendMessage("/null")));

    PyOscManager::get_instance().remove_object(server7->get_name());
//    PyOscManager::get_instance().reallocate_port(server7->get_send_port());
//    PyOscManager::get_instance().reallocate_port(server7->get_recv_port());
    server7.reset();

    thread1->update_status();
    assert(thread1->get_status() == Status::parent_obj_missing);    // parent was deleted

    // Creating parent again
    server7 = PyOscManager::get_instance().new_server(server_name, "/status", "terminate", "127.0.0.1", std::nullopt);
    thread1->set_parent(PyOscManager::get_instance().get_object(thread1->get_parent_name()));

    thread1->update_status();
    assert(thread1->get_status() == Status::parent_obj_not_ready);    // not ready once more

    

}